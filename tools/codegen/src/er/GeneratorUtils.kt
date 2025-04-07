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
    fun <T> ProtocolMessageEnum.getOption(extension: GeneratedExtension<EnumValueOptions, T>): T =
        valueDescriptor.getOption(extension)

    fun <T> EnumValueDescriptor.getOption(extension: GeneratedExtension<EnumValueOptions, T>): T =
        options.getExtension(extension)

    fun <T> FieldDescriptor.getOption(extension: GeneratedExtension<FieldOptions, T>): T =
        toProto().options.getExtension(extension)

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
        ).speciesList.filter { it.randomizerBanned != SPECIES_HIDDEN }
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
}