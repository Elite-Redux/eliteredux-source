package er.defines

import er.Generator
import er.proto.TrainerClass
import er.proto.TrainerMusic
import er.proto.TrainerPic
import java.io.OutputStreamWriter

object TrainerMiscEnumsGenerator : Generator {
    override fun generate(writer: OutputStreamWriter) {
        val trainerPics = TrainerPic.entries.filter { it != TrainerPic.UNRECOGNIZED }
        val trainerMusic = TrainerMusic.entries.filter { it != TrainerMusic.UNRECOGNIZED }
        val trainerClasses = TrainerClass.entries.filter { it != TrainerClass.UNRECOGNIZED }
        writer.appendLine(
            """
            |#ifdef __assembly__
            |
            |${trainerPics.joinToString("\n") { "#define ${it.name} ${it.number}" }}
            |${trainerMusic.joinToString("\n") { "#define ${it.name} ${it.number}" }}
            |${trainerClasses.joinToString("\n") { "#define ${it.name} ${it.number}" }}
            |
            |#else
            |
            |typedef enum TrainerPicEnum {
            |${trainerPics.joinToString("\n") { "    ${it.name} = ${it.number}," }}
            |} TrainerPicEnum;
            |typedef enum TrainerMusicEnum {
            |${trainerMusic.joinToString("\n") { "    ${it.name} = ${it.number}," }}
            |} TrainerMusicEnum;
            |typedef enum TrainerClassEnum {
            |${trainerClasses.joinToString("\n") { "    ${it.name} = ${it.number}," }}
            |} TrainerClassEnum;
            |
            |#endif
            |""".trimMargin()
        )
    }

}