package er

import com.google.protobuf.DescriptorProtos.EnumValueOptions
import com.google.protobuf.DescriptorProtos.FieldOptions
import com.google.protobuf.Descriptors.EnumValueDescriptor
import com.google.protobuf.Descriptors.FieldDescriptor
import com.google.protobuf.GeneratedMessage.GeneratedExtension
import com.google.protobuf.ProtocolMessageEnum
import com.google.protobuf.TextFormat
import er.FileGenerator.IND
import er.proto.*
import er.proto.Species.RandomizeBanned.SPECIES_HIDDEN
import java.io.File
import java.io.OutputStreamWriter

object GeneratorUtils {

    val ITEMS_LIST by lazy {
        (Pocket.entries.filter { it != Pocket.POCKET_NONE && it != Pocket.UNRECOGNIZED }.map {
            it.name.removePrefix("POCKET_").split("_")
                .joinToString("") { s -> s.lowercase().replaceFirstChar { c -> c.uppercaseChar() } }
        } + "Unused").flatMap {
            if (File("../../proto/items/${it}List.textproto").exists()) {
                TextFormat.parse(
                    File("../../proto/items/${it}List.textproto").readText(),
                    ItemList::class.java
                ).itemList
            } else {
                emptyList()
            }
        }
    }

    val ABILITIES_LIST by lazy {
        TextFormat.parse(File("../../proto/AbilityList.textproto").readText(), AbilityList::class.java).abilityList
    }

    val MOVES_LIST by lazy {
        TextFormat.parse(File("../../proto/MoveList.textproto").readText(), MoveList::class.java).movesList
    }

    val SPECIES_LIST by lazy {
        TextFormat.parse(
            File("../../proto/SpeciesList.textproto").readText(),
            SpeciesList::class.java
        ).speciesList.filter { it.randomizerBanned != SPECIES_HIDDEN && it.spe > 0 }
    }

    val NO_EGG_LIST by lazy {
        SPECIES_LIST.filter { it.id != SpeciesEnum.SPECIES_EGG }
    }

    val SPECIES_COUNT by lazy {
        SPECIES_LIST.maxOf { it.id.number } + 1
    }

    val REAL_SPECIES_COUNT by lazy {
        NO_EGG_LIST.maxOf { it.id.number } + 1
    }

    val SPECIES_MAP by lazy {
        SPECIES_LIST.associateBy { it.id }
    }

    fun Species.resolveVisuals(): Visuals =
        if (hasReuseVisuals()) SPECIES_MAP[reuseVisuals]!!.resolveVisuals() else this.visuals

    fun <T> ProtocolMessageEnum.getOption(extension: GeneratedExtension<EnumValueOptions, T>): T =
        valueDescriptor.getOption(extension)

    fun <T> EnumValueDescriptor.getOption(extension: GeneratedExtension<EnumValueOptions, T>): T =
        options.getExtension(extension)

    fun <T> FieldDescriptor.getOption(extension: GeneratedExtension<FieldOptions, T>): T =
        toProto().options.getExtension(extension)

    fun String.fileID() =
        lowercase().replace("[^a-z0-9_]".toRegex()) { "E${it.value.replace("\\", "/").single().code}" }

    /**
     * Takes a list of key-value pairs and creates a set of mappings of keys/values to a shared index value.
     */
    fun <Key, Value> List<Pair<Key, Value>>.createDedupMaps(): Pair<Map<Key, Int>, Map<Value, Int>> {
        val groups = groupBy({ it.first }, { it.second })

        val keyIds = groups.keys.withIndex().associate { it.value to it.index }
        val valueIds =
            groups.flatMap { (key, values) -> values.map { it to keyIds[key]!! } }
                .toMap()
        return keyIds to valueIds
    }

    fun Map<*, Int>.printLookupTable(tableSignature: String, prefix: String, writer: OutputStreamWriter) {
        writer.appendLine(
            """
            |$tableSignature = {
            |$IND${entries.joinToString("\n$IND") { "[${it.key}] = $prefix${it.value}," }}
            |};
            |""".trimMargin()
        )
    }

    enum class PrintMode { FILE, STRING }

    /**
     * Lookup table definition for deduping
     *
     * @param signature signature for the table to print, will have const prepended and '= {' appended
     * @param data list of keys to string values
     * @param entryFunction transforms a key plus dedup entry name to a table entry. Has indentation prepended and ',' appended.
     */
    data class LookupTable<T>(
        val signature: String,
        val data: List<Pair<T, String?>>,
        val entryFunction: (T, String) -> String = { key, name -> "[$key] = $name" }
    )

    /**
     * Prints a set of lookup tables to string values with deduplicated strings.
     *
     * @param mode whether the string value is a filepath or a display string
     * @param prefix prefix of the static dedup entries
     * @param valueFunction method that generates the dedup entry, (field, string) -> code. Will be prepended with static const and appended with ;.
     * @param tables table entries to include. Entries included across tables will be deduped.
     */
    fun <T> printLookupTables(
        writer: OutputStreamWriter,
        mode: PrintMode,
        prefix: String,
        valueFunction: (String, String) -> String,
        vararg tables: LookupTable<T>
    ) {
        fun String.dedupEntry() = prefix + if (mode == PrintMode.FILE) fileID() else TODO("String mode not implemented")

        tables.asSequence().flatMap { it.data }.map { it.second }.filterNotNull().filter { it.isNotBlank() }
            .associateBy { it.dedupEntry() }.entries.forEach {
                writer.appendLine("static const ${valueFunction(it.key, it.value)};")
            }

        for (table in tables) {
            writer.appendLine("const ${table.signature} = {")
            table.data.filter { !it.second.isNullOrBlank() }
                .forEach { writer.appendLine("$IND${table.entryFunction(it.first, it.second!!.dedupEntry())},") }
            writer.appendLine("};\n")
        }
    }
}