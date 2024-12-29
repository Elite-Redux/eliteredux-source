package er

import com.google.protobuf.TextFormat
import er.FileGenerator.IND
import er.proto.SpeciesEnum
import er.proto.SpeciesList
import java.io.File
import java.io.FileWriter
import java.io.OutputStreamWriter

object GeneratorUtils {
    val SPECIES_LIST by lazy {
        TextFormat.parse(File("../../proto/SpeciesList.textproto").readText(), SpeciesList::class.java).speciesList
    }

    val NO_EGG_LIST by lazy {
        SPECIES_LIST.filter { it.id != SpeciesEnum.SPECIES_EGG }
    }

    val SPECIES_COUNT by lazy {
        SpeciesEnum.entries.filter {
            when (it) {
                SpeciesEnum.UNRECOGNIZED, SpeciesEnum.SPECIES_NONE -> false
                else -> true
            }
        }.maxOf { it.number } + 1
    }

    val REAL_SPECIES_COUNT by lazy {
        SpeciesEnum.entries.filter {
            when (it) {
                SpeciesEnum.UNRECOGNIZED, SpeciesEnum.SPECIES_NONE, SpeciesEnum.SPECIES_EGG -> false
                else -> true
            }
        }.maxOf { it.number } + 1
    }

    val SPECIES_MAP by lazy {
        SPECIES_LIST.associateBy { it.id }
    }

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