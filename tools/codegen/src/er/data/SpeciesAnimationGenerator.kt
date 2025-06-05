package er.data

import er.FileGenerator.IND
import er.Generator
import er.GeneratorUtils.SPECIES_LIST
import er.GeneratorUtils.SPECIES_MAP
import er.GeneratorUtils.createDedupMaps
import er.GeneratorUtils.printLookupTable
import er.data.SpeciesAnimationGenerator.toStruct
import er.proto.Species
import er.proto.SpeciesEnum.SPECIES_NONE
import er.proto.Visuals.AnimationList
import er.proto.Visuals.AnimationList.AnimCmd
import er.proto.Visuals.AnimationList.AnimStep
import er.proto.VisualsKt.AnimationListKt.animCmd
import er.proto.VisualsKt.AnimationListKt.animStep
import er.proto.VisualsKt.animationList
import java.io.OutputStreamWriter
import kotlin.collections.Map.Entry

object SpeciesAnimationGenerator : Generator {
    private fun getAnimation(species: Species): AnimationList = when {
        species.reuseVisuals != SPECIES_NONE -> getAnimation(SPECIES_MAP[species.reuseVisuals]!!)
        species.visuals.reuseAnimation != SPECIES_NONE -> getAnimation(SPECIES_MAP[species.visuals.reuseAnimation]!!)
        else -> species.visuals.anim.takeIf { it.stepList.isNotEmpty() }
            ?: animationList { step += animStep { frame += animCmd { duration = 1 } } }
    }

    private const val STEP_PREFIX = "__sAnimStep_"
    private const val LIST_PREFIX = "__sAnimList_"

    private fun AnimCmd.toStruct() = when (type) {
        AnimCmd.Type.FRAME -> "ANIMCMD_FRAME($image, $duration)"
        AnimCmd.Type.JUMP -> TODO("Jump not support yet")
        AnimCmd.Type.LOOP -> TODO("Loop not supported yet")
        AnimCmd.Type.UNRECOGNIZED -> error("Unrecognized AnimCmd type")
    }

    private fun Entry<AnimStep, Int>.toStruct() =
        """
        |static const union AnimCmd $STEP_PREFIX$value[] = {
        |$IND${key.frameList.joinToString("\n$IND") { it.toStruct() + "," }}
        |${IND}ANIMCMD_END,
        |};
        |""".trimMargin()

    private fun Entry<AnimationList, Int>.toStruct(stepMap: Map<AnimStep, Int>) =
        """
        |static const union AnimCmd *const $LIST_PREFIX$value[] = {
        |${IND}sAnim_GeneralFrame0,
        |$IND${key.stepList.joinToString("\n$IND") { "$STEP_PREFIX${stepMap[it]}," }}
        |};
        |""".trimMargin()

    override fun generate(writer: OutputStreamWriter) {
        val animationLists = SPECIES_LIST.map { getAnimation(it) to it.id }
        val animationSteps = animationLists.flatMap { (animation, _) -> animation.stepList.map { it to animation } }

        val animationStepsToId = animationSteps.createDedupMaps().first
        val (animationListToId, speciesToId) = animationLists.createDedupMaps()

        writer.appendLine(
            """
            |${animationStepsToId.entries.joinToString("\n") { it.toStruct() }}
            |
            |${animationListToId.entries.joinToString("\n") { it.toStruct(animationStepsToId) }}
            |""".trimMargin()
        )

        speciesToId.printLookupTable("const union AnimCmd *const *const gMonFrontAnimsPtrTable[]", LIST_PREFIX, writer)
    }
}