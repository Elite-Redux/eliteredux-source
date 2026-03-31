package er.data

import er.Generator
import java.io.OutputStreamWriter

object BattleFrontierDefinesGenerator : Generator {
  override fun generate(writer: OutputStreamWriter) {
    writer.appendLine(
      "#define FRONTIER_MONS_COUNT ${BattleFrontierMonGenerator.SETS.size + BattleFrontierMonGenerator.RESTRICTED_SETS.size}"
    )
    writer.appendLine("#define FRONTIER_MONS_HIGH_TIER ${BattleFrontierMonGenerator.SETS.size}")
  }
}
