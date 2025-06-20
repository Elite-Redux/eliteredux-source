package er.defines

import er.Generator
import er.proto.TrainerEnum
import java.io.OutputStreamWriter

object TrainerEnumGenerator : Generator {
    override fun generate(writer: OutputStreamWriter) {
        val trainers = TrainerEnum.entries.filter { it != TrainerEnum.UNRECOGNIZED }
        val trainerCount = trainers.filter { it != TrainerEnum.TRAINER_OLDPLAYER }.maxOf { it.number } + 1
        writer.appendLine(
            """
            |#ifdef __assembly__
            |
            |${trainers.joinToString("\n") { "#define ${it.name} ${it.number}" }}
            |
            |#define TRAINERS_COUNT $trainerCount
            |
            |#else
            |
            |typedef enum TrainerEnum {
            |${trainers.joinToString("\n") { "    ${it.name} = ${it.number}," }}
            |    TRAINERS_COUNT = $trainerCount,
            |} TrainerEnum;
            |
            |#endif
            |""".trimMargin()
        )
    }
}