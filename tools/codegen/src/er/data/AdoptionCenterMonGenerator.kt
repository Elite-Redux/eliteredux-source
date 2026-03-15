package er.data

import er.FileGenerator.IND
import er.Generator
import er.GeneratorUtils.SPECIES_LIST
import er.proto.Species
import er.proto.SpeciesEnum
import java.io.OutputStreamWriter

object AdoptionCenterMonGenerator : Generator {
  override fun generate(writer: OutputStreamWriter) {
    fun idOf(species: Species): SpeciesEnum =
      if (species.hasFormOf()) species.formOf else species.id

    val adoptionCenterMons =
      SPECIES_LIST.filter { it.hasAdoptionCenterBadge() }
        .groupBy { it.adoptionCenterBadge }
        .mapValues { it.value.sortedBy { idOf(it) } }
        .toSortedMap()

    val monBadgeCounts = buildList {
      add(adoptionCenterMons[0].orEmpty().size)
      for (badge in 1..8) {
        add(adoptionCenterMons[badge].orEmpty().size + this.last())
      }
    }

    writer.appendLine(
      """
      |static const SpeciesEnum sAdoptionCenterInventories[] = {
      |$IND${adoptionCenterMons.entries.joinToString(",\n$IND") { it.value.joinToString(prefix = "// ${it.key} badges\n$IND", separator = ",\n$IND") { mon -> mon.id.name } } }
      |};
      |
      |static const u8 sAdoptionCenterCountsByBadge[] = {${monBadgeCounts.joinToString()}};
      |"""
        .trimMargin()
    )
  }
}
