package er.defines

import er.FileGenerator.IND
import er.Generator
import java.io.OutputStreamWriter

abstract class EnumGenerator(
  private val name: String,
  private val keyMap: Map<String, Int>,
  private val countName: String? = null,
  private val forEachName: String? = null,
) : Generator {

  override fun generate(writer: OutputStreamWriter) {
    val sortedEntries = keyMap.entries.sortedBy { it.value }

    writer.appendLine(
      """
        |
        |#ifdef __assembly__
        |
        |${sortedEntries.joinToString("\n") { "#define ${it.key} ${it.value}" }}
        |${countName?.let { "#define $countName ${sortedEntries.maxOf { it.value } + 1}" }.orEmpty()}
        |
        |#else
        |
        |typedef enum $name {
        |$IND${sortedEntries.joinToString("\n$IND") { "${it.key} = ${it.value}," }}
        |${countName?.let { "$IND$countName = ${sortedEntries.maxOf { it.value } + 1}" }.orEmpty()}
        |} $name;
        |
        |#endif
        |"""
        .trimMargin()
    )

    if (forEachName != null) {
      val gaps =
        sortedEntries.zipWithNext().filterNot { (first, second) -> first.value + 1 == second.value }
      check(gaps.isEmpty()) {
        "Gaps found in list, FOR_EACH_FUNCTION will not function appropriately: $gaps"
      }

      writer.appendLine(
        """
          |#define $forEachName \
          |$IND${sortedEntries.joinToString(" \\\n$IND") { "${forEachName}_FUNCTION(${it.key})"}}
          |"""
          .trimMargin()
      )
    }
  }
}
