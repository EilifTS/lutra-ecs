#pragma once
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <lutra-ecs/save/ByteIO.h>
#include <lutra-ecs/save/Crc32.h>
#include <lutra-ecs/save/Error.h>
#include <lutra-ecs/save/Schema.h>

/**
 *  A save as data, and the bytes it goes to and comes from. The container holds
 *  what every save of an ECS has -- the entities, the components of each saved
 *  type, tables of names -- and room for whatever else a game has to put beside
 *  them. Nothing here knows about a particular game.
 *
 *  Layout, all little-endian:
 *
 *    header       magic u32 | format_version u32        (both the game's: see Format)
 *    entities     count u32 | EntityID u32 * count       (strictly ascending by index)
 *    names        table_count u32 | for each table:
 *                   count u32 | { value u32 | name (length u32 + bytes) } * count
 *    sections     count u32 | { section_id u32 | length u32 | bytes } * count
 *    components   chunk_count u32 | chunks, each:
 *                   component_id u32 | size u32 | count u32 |
 *                   EntityID u32 * count | component bytes (size * count)
 *    trailer      CRC-32 of every byte before it
 *
 *  A name table maps a number a component holds (a handle into some library) to
 *  the name that number stood for, so a save survives the library being numbered
 *  differently. A section is a block of the game's own bytes, found by id; a
 *  section a build does not look for is carried along and ignored. A chunk of a
 *  component this build does not know is skipped by its length. A chunk of one it
 *  does know, with another size, is an error: the layout changed and the bytes
 *  cannot be reinterpreted.
 */
namespace lcs::save
{
	/* What identifies the file and how it is read. Both numbers are the game's. */
	struct Format
	{
		uint32_t magic{};
		uint32_t version{};       /* Written; a file of another version is refused. */
		size_t name_table_count{ 0 };
		size_t max_name_length{ 1024 };
	};

	/* handle value -> name */
	using NameTable = std::unordered_map<uint32_t, std::string>;

	/* A block of the game's own bytes. */
	struct Section
	{
		uint32_t id{};
		std::vector<std::byte> bytes{};
	};

	/* Every saved component of one type, as bytes. */
	template <typename EntityID>
	struct ComponentChunk
	{
		uint32_t component_id{};
		uint32_t size{};                  /* Bytes of one component; 0 for a tag. */
		std::vector<EntityID> entities{}; /* Which entity each component belongs to. */
		std::vector<std::byte> bytes{};   /* size * entities.size() bytes, in the same order. */

		[[nodiscard]] size_t Count() const { return entities.size(); }

		[[nodiscard]] std::span<const std::byte> Component(const size_t index) const
		{
			return std::span<const std::byte>(bytes).subspan(index * size, size);
		}
	};

	template <typename EntityID>
	struct Container
	{
		std::vector<EntityID> entities{};
		std::vector<NameTable> names{};      /* Format::name_table_count of them. */
		std::vector<Section> sections{};
		std::vector<ComponentChunk<EntityID>> chunks{};

		/* Only Parse fills this: the ids of chunks it skipped because this build has
		 * no such component. The caller reports them. */
		std::vector<uint32_t> skipped_component_ids{};
	};

	/* The chunk for `entry`, made if there is none yet. */
	template <typename EntityID>
	ComponentChunk<EntityID>& ChunkFor(Container<EntityID>& data, const SchemaEntry& entry)
	{
		for (ComponentChunk<EntityID>& chunk : data.chunks)
		{
			if (chunk.component_id == entry.id)
			{
				return chunk;
			}
		}

		data.chunks.push_back(ComponentChunk<EntityID>{ .component_id = entry.id, .size = entry.size });
		return data.chunks.back();
	}

	template <typename EntityID>
	[[nodiscard]] const ComponentChunk<EntityID>* FindChunk(const Container<EntityID>& data, const uint32_t component_id)
	{
		for (const ComponentChunk<EntityID>& chunk : data.chunks)
		{
			if (chunk.component_id == component_id)
			{
				return &chunk;
			}
		}
		return nullptr;
	}

	template <typename EntityID>
	[[nodiscard]] const Section* FindSection(const Container<EntityID>& data, const uint32_t section_id)
	{
		for (const Section& section : data.sections)
		{
			if (section.id == section_id)
			{
				return &section;
			}
		}
		return nullptr;
	}

	/* `Schema` is the lcs::save::Schema the component belongs to. */
	template <typename Schema, typename EntityID, typename T>
	void AddComponent(Container<EntityID>& data, const EntityID entity, const T& component)
	{
		static_assert(T::component_type != ComponentType::Tag, "a tag has no bytes, use AddTag");

		ComponentChunk<EntityID>& chunk = ChunkFor(data, Schema::template EntryOf<T>());
		chunk.entities.push_back(entity);

		const std::span<const std::byte> bytes = std::as_bytes(std::span(&component, 1));
		chunk.bytes.insert(chunk.bytes.end(), bytes.begin(), bytes.end());
	}

	template <typename Schema, typename T, typename EntityID>
	void AddTag(Container<EntityID>& data, const EntityID entity)
	{
		static_assert(T::component_type == ComponentType::Tag);

		ChunkFor(data, Schema::template EntryOf<T>()).entities.push_back(entity);
	}

	template <typename T, typename EntityID>
	[[nodiscard]] T ReadComponent(const ComponentChunk<EntityID>& chunk, const size_t index)
	{
		T component;
		std::memcpy(&component, chunk.Component(index).data(), sizeof(T));
		return component;
	}

	/*
	 * Records: a section that is an array of one fixed-size type
	 *
	 *    record_size u32 | count u32 | Record * count
	 */

	template <typename Record>
	[[nodiscard]] Section MakeRecordSection(const uint32_t section_id, const std::span<const Record> records)
	{
		ByteWriter writer;
		writer.U32(uint32_t(sizeof(Record)));
		writer.U32(uint32_t(records.size()));
		writer.RawArray<Record>(records);
		return Section{ .id = section_id, .bytes = writer.Take() };
	}

	/* The records, checked only for their size and count: what is in them is for
	 * the caller to check. `what` names them in an error. */
	template <typename Record>
	[[nodiscard]] Result<std::vector<Record>> ReadRecordSection(const Section& section, const char* what)
	{
		ByteReader reader(section.bytes);

		const Result<uint32_t> record_size = reader.U32();
		if (!record_size) return std::unexpected(record_size.error());
		if (*record_size != sizeof(Record))
		{
			return std::unexpected(MakeError(ErrorKind::SizeMismatch, std::string("the ") + what + " records are " + std::to_string(*record_size) + " bytes, this build's are " + std::to_string(sizeof(Record))));
		}

		const Result<uint32_t> count = reader.Count(sizeof(Record));
		if (!count) return std::unexpected(count.error());
		const Result<std::span<const std::byte>> bytes = reader.Bytes(size_t(*count) * sizeof(Record));
		if (!bytes) return std::unexpected(bytes.error());
		if (!reader.AtEnd())
		{
			return std::unexpected(MakeError(ErrorKind::Malformed, std::string("there are bytes after the last ") + what + " record"));
		}

		std::vector<Record> records(*count);
		if (*count > 0)
		{
			std::memcpy(records.data(), bytes->data(), bytes->size());
		}
		return records;
	}

	/*
	 * Encode
	 */

	template <typename EntityID>
	[[nodiscard]] std::vector<std::byte> Encode(const Container<EntityID>& data, const Format& format)
	{
		static_assert(sizeof(EntityID::handle) == sizeof(uint32_t), "entity ids are written as 32 bits");
		assert(data.names.size() == format.name_table_count && "the container has another number of name tables than the format says");

		ByteWriter writer;
		writer.U32(format.magic);
		writer.U32(format.version);

		writer.U32(uint32_t(data.entities.size()));
		for (const EntityID entity : data.entities)
		{
			writer.U32(entity.handle);
		}

		writer.U32(uint32_t(data.names.size()));
		for (const NameTable& table : data.names)
		{
			/* By handle value, so the same save is written the same way every time. */
			std::vector<std::pair<uint32_t, const std::string*>> entries;
			for (const auto& [value, name] : table)
			{
				entries.emplace_back(value, &name);
			}
			std::ranges::sort(entries, {}, &std::pair<uint32_t, const std::string*>::first);

			writer.U32(uint32_t(entries.size()));
			for (const auto& [value, name] : entries)
			{
				writer.U32(value);
				writer.String(*name);
			}
		}

		writer.U32(uint32_t(data.sections.size()));
		for (const Section& section : data.sections)
		{
			writer.U32(section.id);
			writer.U32(uint32_t(section.bytes.size()));
			writer.Bytes(section.bytes);
		}

		writer.U32(uint32_t(data.chunks.size()));
		for (const ComponentChunk<EntityID>& chunk : data.chunks)
		{
			writer.U32(chunk.component_id);
			writer.U32(chunk.size);
			writer.U32(uint32_t(chunk.entities.size()));
			for (const EntityID entity : chunk.entities)
			{
				writer.U32(entity.handle);
			}
			writer.Bytes(chunk.bytes);
		}

		writer.U32(Crc32(writer.View()));
		return writer.Take();
	}

	/*
	 * Parse
	 */

	namespace detail
	{
		/* `expression` is a Result; hands the error on, or gives the value. */
#define LCS_SAVE_TRY(name, expression) \
		const auto name##_result = (expression); \
		if (!name##_result) return std::unexpected(name##_result.error()); \
		const auto& name = *name##_result

		template <typename EntityID>
		bool IsUsableId(const EntityID entity)
		{
			return entity.IsValid() && entity.GetIndex() < EntityID::max_index;
		}

		template <typename EntityID>
		Result<std::vector<EntityID>> ReadEntities(ByteReader& reader)
		{
			LCS_SAVE_TRY(count, reader.Count(sizeof(uint32_t)));

			std::vector<EntityID> entities;
			entities.reserve(count);
			for (uint32_t n = 0; n < count; n++)
			{
				LCS_SAVE_TRY(handle, reader.U32());
				const EntityID entity{ handle };

				if (!IsUsableId(entity))
				{
					return std::unexpected(MakeError(ErrorKind::Malformed, "an entity id is not a valid id"));
				}
				if (!entities.empty() && entities.back().GetIndex() >= entity.GetIndex())
				{
					return std::unexpected(MakeError(ErrorKind::Malformed, "the entity ids are not strictly ascending"));
				}

				entities.push_back(entity);
			}
			return entities;
		}

		inline Result<NameTable> ReadNameTable(ByteReader& reader, const size_t table, const size_t max_name_length)
		{
			LCS_SAVE_TRY(count, reader.Count(2 * sizeof(uint32_t)));

			NameTable names;
			for (uint32_t n = 0; n < count; n++)
			{
				LCS_SAVE_TRY(value, reader.U32());
				LCS_SAVE_TRY(name, reader.String(max_name_length));

				if (name.empty())
				{
					return std::unexpected(MakeError(ErrorKind::Malformed, "name table " + std::to_string(table) + " has an empty name"));
				}
				if (!names.emplace(value, name).second)
				{
					return std::unexpected(MakeError(ErrorKind::Malformed, "name table " + std::to_string(table) + " names the same handle twice"));
				}
			}
			return names;
		}

		inline Result<std::vector<Section>> ReadSections(ByteReader& reader)
		{
			LCS_SAVE_TRY(count, reader.Count(2 * sizeof(uint32_t)));

			std::vector<Section> sections;
			std::unordered_set<uint32_t> seen;
			for (uint32_t n = 0; n < count; n++)
			{
				LCS_SAVE_TRY(id, reader.U32());
				LCS_SAVE_TRY(length, reader.U32());
				LCS_SAVE_TRY(bytes, reader.Bytes(length));

				if (!seen.insert(id).second)
				{
					return std::unexpected(MakeError(ErrorKind::Malformed, "section " + std::to_string(id) + " is in the file twice"));
				}

				sections.push_back(Section{ .id = id, .bytes = std::vector<std::byte>(bytes.begin(), bytes.end()) });
			}
			return sections;
		}

		/* Reads the chunk after `component_id`, `size` and `count`. */
		template <typename EntityID>
		Result<ComponentChunk<EntityID>> ReadChunkBody(ByteReader& reader, const uint32_t component_id, const uint32_t size, const uint32_t count)
		{
			ComponentChunk<EntityID> chunk{ .component_id = component_id, .size = size };
			chunk.entities.reserve(count);

			std::unordered_set<uint32_t> seen;
			for (uint32_t n = 0; n < count; n++)
			{
				LCS_SAVE_TRY(handle, reader.U32());
				const EntityID entity{ handle };

				if (!IsUsableId(entity))
				{
					return std::unexpected(MakeError(ErrorKind::Malformed, "a component belongs to an entity id that is not a valid id"));
				}
				if (!seen.insert(handle).second)
				{
					return std::unexpected(MakeError(ErrorKind::Malformed, "an entity has the same component twice"));
				}

				chunk.entities.push_back(entity);
			}

			LCS_SAVE_TRY(bytes, reader.Bytes(size_t(count) * size));
			chunk.bytes.assign(bytes.begin(), bytes.end());
			return chunk;
		}
	}

	/* Every byte is untrusted. What this checks is the structure: the header, the
	 * checksum, every count against what is left, the size of every component
	 * against the schema's, the entity ids. It does not look at what the
	 * components or the sections say, and touches nothing but the bytes. */
	template <typename EntityID>
	[[nodiscard]] Result<Container<EntityID>> Parse(const std::span<const std::byte> bytes, const Format& format, const std::span<const SchemaEntry> schema)
	{
		constexpr size_t header_size = 2 * sizeof(uint32_t);
		constexpr size_t trailer_size = sizeof(uint32_t);

		if (bytes.size() < header_size + trailer_size)
		{
			return std::unexpected(MakeError(ErrorKind::Truncated, "the file is too short to be a save"));
		}

		ByteReader reader(bytes.first(bytes.size() - trailer_size));

		LCS_SAVE_TRY(magic, reader.U32());
		if (magic != format.magic)
		{
			return std::unexpected(MakeError(ErrorKind::BadMagic, "this is not a save file"));
		}

		LCS_SAVE_TRY(version, reader.U32());
		if (version != format.version)
		{
			return std::unexpected(MakeError(ErrorKind::UnsupportedVersion, "the save is version " + std::to_string(version) + ", this build reads version " + std::to_string(format.version)));
		}

		uint32_t stored_checksum;
		std::memcpy(&stored_checksum, bytes.data() + bytes.size() - trailer_size, trailer_size);
		if (Crc32(bytes.first(bytes.size() - trailer_size)) != stored_checksum)
		{
			return std::unexpected(MakeError(ErrorKind::BadChecksum, "the save is damaged: its checksum does not match"));
		}

		Container<EntityID> data;

		LCS_SAVE_TRY(entities, detail::ReadEntities<EntityID>(reader));
		data.entities = entities;

		LCS_SAVE_TRY(table_count, reader.Count(sizeof(uint32_t)));
		if (table_count != format.name_table_count)
		{
			return std::unexpected(MakeError(ErrorKind::SizeMismatch, "the save has " + std::to_string(table_count) + " name tables, this build's has " + std::to_string(format.name_table_count)));
		}
		for (size_t table = 0; table < format.name_table_count; table++)
		{
			LCS_SAVE_TRY(names, detail::ReadNameTable(reader, table, format.max_name_length));
			data.names.push_back(names);
		}

		LCS_SAVE_TRY(sections, detail::ReadSections(reader));
		data.sections = sections;

		constexpr size_t chunk_header_size = 3 * sizeof(uint32_t);
		LCS_SAVE_TRY(chunk_count, reader.Count(chunk_header_size));
		for (uint32_t n = 0; n < chunk_count; n++)
		{
			LCS_SAVE_TRY(component_id, reader.U32());
			LCS_SAVE_TRY(size, reader.U32());
			LCS_SAVE_TRY(count, reader.U32());

			/* Each component takes its entity id and its bytes. */
			const uint64_t per_component = uint64_t(sizeof(uint32_t)) + size;
			if (count > reader.Remaining() / per_component)
			{
				return std::unexpected(MakeError(ErrorKind::BadCount, "a component chunk holds more than the rest of the file"));
			}

			const SchemaEntry* entry = nullptr;
			for (const SchemaEntry& candidate : schema)
			{
				if (candidate.id == component_id) { entry = &candidate; break; }
			}

			if (entry == nullptr)
			{
				LCS_SAVE_TRY(skipped, reader.Bytes(size_t(count * per_component)));
				(void)skipped;
				data.skipped_component_ids.push_back(component_id);
				continue;
			}

			if (size != entry->size)
			{
				return std::unexpected(MakeError(ErrorKind::SizeMismatch, std::string(entry->name) + " is " + std::to_string(size) + " bytes in the file, this build's is " + std::to_string(entry->size)));
			}
			if (FindChunk(data, component_id) != nullptr)
			{
				return std::unexpected(MakeError(ErrorKind::Malformed, std::string(entry->name) + " has two chunks"));
			}

			LCS_SAVE_TRY(chunk, detail::ReadChunkBody<EntityID>(reader, component_id, size, count));
			data.chunks.push_back(chunk);
		}

		if (!reader.AtEnd())
		{
			return std::unexpected(MakeError(ErrorKind::Malformed, "there are bytes after the last chunk"));
		}

		return data;
	}

#undef LCS_SAVE_TRY
}
