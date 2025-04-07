package er.data

import er.FileGenerator.IND
import er.Generator
import er.GeneratorUtils.SPECIES_LIST
import java.io.OutputStreamWriter

object UndoFormChangeGenerator : Generator {
    override fun generate(writer: OutputStreamWriter) {
        writer.appendLine(
            """
            |const SpeciesEnum gUndoFormChangeTable[][3] = {
            |$IND${
                SPECIES_LIST.filter { it.hasBattleForm() }
                    .joinToString("\n$IND") { "{${it.id}, ${it.battleForm.of}, ${if (it.battleForm.keepOnSwitch) "FALSE" else "TRUE"}}," }
            }
            |};
            |""".trimMargin()
        )
    }
}