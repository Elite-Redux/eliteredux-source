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
    writer.appendLine(
      """
        |
        |#ifdef __assembly__
        |
        |${keyMap.entries.joinToString("\n") { "#define ${it.key} ${it.value}" }}
        |${countName?.let { "#define $countName ${keyMap.values.max() + 1}" }.orEmpty()}
        |
        |#else
        |
        |typedef enum $name {
        |$IND${keyMap.entries.joinToString("\n$IND") { "${it.key} = ${it.value}," }}
        |${countName?.let { "$IND$countName = ${keyMap.values.max() + 1}" }.orEmpty()}
        |} $name;
        |
        |#endif
        |"""
        .trimMargin()
    )

    if (forEachName != null) {
      writer.appendLine(
        """
          |#define $forEachName \
          |$IND${keyMap.keys.joinToString(" \\\n$IND") { "${forEachName}_FUNCTION($it)"}}
          |"""
          .trimMargin()
      )
    }
  }
}
