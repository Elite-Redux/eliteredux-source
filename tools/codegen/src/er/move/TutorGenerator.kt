package er.move

import er.Generator
import er.GeneratorUtils.MOVES_LIST
import er.proto.Move
import er.proto.MoveSplit.STATUS
import er.proto.TutorType.*
import java.io.OutputStreamWriter

object TutorGenerator : Generator {
    private fun List<Move>.decorators() = sortedBy { it.name }. joinToString("\n") { "TUTOR_DECORATOR(${it.id}) \\" }

    override fun generate(writer: OutputStreamWriter) {
        val tutors = MOVES_LIST.filter { it.tutor != TUTOR_NONE } .groupBy { it.tutor }
        for ((type, move) in tutors.entries) {
            if (type == TUTOR_ATTACK || type == TUTOR_UTILITY_ATTACK || type == TUTOR_UNIVERSAL_ATTACK) {
                move.forEach { check(it.split != STATUS) {"Tutor move is flagged as $type but is not attack: $it"} }
            } else {
                move.forEach { check(it.split == STATUS) {"Tutor move is flagged as $type but is attack: $it"} }
            }
        }

        writer.appendLine(
            """
            |#define ALL_TUTORS \
            |${(tutors[TUTOR_ATTACK].orEmpty() + tutors[TUTOR_UNIVERSAL_ATTACK].orEmpty()).decorators()}
            |${tutors[TUTOR_UTILITY_ATTACK].orEmpty().decorators()}
            |${tutors[TUTOR_GOOD_STATUS].orEmpty().decorators()}
            |${tutors[TUTOR_NICHE_STATUS].orEmpty().decorators()}
            |${(tutors[TUTOR_UNIVERSAL_STATUS].orEmpty() + tutors[TUTOR_UNIVERSAL_STATUS_GENDERED].orEmpty()).decorators()}
            |${tutors[TUTOR_TERRAIN].orEmpty().decorators()}
            |${tutors[TUTOR_WEATHER].orEmpty().decorators()}
            |""".trimMargin()
        )
    }
}