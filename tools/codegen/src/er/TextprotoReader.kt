package er

import com.google.protobuf.TextFormat
import er.proto.SpeciesEnum
import er.proto.SpeciesList
import java.io.File

object TextprotoReader {
    val SPECIES_LIST by lazy {
        TextFormat.parse(File("../../proto/SpeciesList.textproto").readText(), SpeciesList::class.java).speciesList
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
}