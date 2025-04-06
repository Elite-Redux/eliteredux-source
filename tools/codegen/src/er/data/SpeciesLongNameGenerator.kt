package er.data

import er.FileGenerator.IND
import er.Generator
import er.GeneratorUtils.SPECIES_LIST
import er.proto.Species
import er.proto.Species.Region.REGION_NONE
import java.io.OutputStreamWriter

object SpeciesLongNameGenerator : Generator {
    override fun generate(writer: OutputStreamWriter) {
        val longNames = SPECIES_LIST.filter { it.longName.isNotBlank() }.map { it.id to it.longName }

        writer.appendLine(
            """
            |MegaType GetSpeciesMegaType(SpeciesEnum species) {
            |${IND}switch (species) {
            |$IND$IND${
                SPECIES_LIST.filter { it.megaList.isNotEmpty() }
                    .joinToString("\n$IND$IND") { "case ${it.id}: return MEGA_${it.megaList.first().type};" }
            }
            |$IND$IND${
                SPECIES_LIST.filter { it.primalList.isNotEmpty() }
                    .joinToString("\n$IND$IND") { "case ${it.id}: return MEGA_${it.primalList.first().type};" }
            }
            |$IND}
            |${IND}return MEGA_NONE;
            |}
            |
            |RegionType GetSpeciesRegionType(SpeciesEnum species) {
            |${IND}switch (species) {
            |$IND$IND${SPECIES_LIST.filter { it.regionPrefix != REGION_NONE }.joinToString("\n$IND$IND") { "case ${it.id}: return REGION_${it.regionPrefix};" }}
            |$IND}
            |${IND}return REGION_NONE;
            |}
            |
            |${longNames.joinToString("\n") { "const u8 __sLongName_${it.first}[] = _(\"${it.second}\");" }}
            |
            |const u8* GetSpeciesLongName(SpeciesEnum species) {
            |${IND}switch (species) {
            |$IND$IND${longNames.joinToString("\n$IND$IND") { "case ${it.first}: return __sLongName_${it.first};" }}
            |$IND}
            |${IND}return NULL;
            |}
            |""".trimMargin()
        )
    }
}