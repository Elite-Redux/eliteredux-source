package er

import er.GeneratorUtils.FULL_SPECIES_LIST
import er.proto.SpeciesKt
import er.proto.Visuals
import er.proto.copy
import er.proto.speciesList
import java.awt.Color
import java.awt.image.BufferedImage
import java.awt.image.ColorModel
import java.awt.image.DataBuffer
import java.awt.image.IndexColorModel
import java.awt.image.MultiPixelPackedSampleModel
import java.awt.image.Raster
import java.io.File
import javax.imageio.ImageIO
import kotlin.math.abs
import kotlin.math.roundToInt

object Palettizer {
  @JvmStatic
  fun main(args: Array<String>) {
    val newSpeciesList = speciesList {
      for (species in FULL_SPECIES_LIST.sortedBy { it.id.number }) {
        this.species += species.copy { fixIcon() }
      }
    }

    File("../../proto/SpeciesList.textproto")
      .writeText(
        """
                |# proto-file: SpeciesList.proto
                |# proto-message: er.SpeciesList
                |
                |$newSpeciesList
                |"""
          .trimMargin()
          .replace(".0", "")
      )
  }

  private fun SpeciesKt.Dsl.fixIcon() {
    val icon = visuals.icon
    val femIcon = visuals.female.icon

    if (icon.path.isEmpty()) return

    indexImage(icon)
    findCorrectPalette(icon)?.let {
      icon.reindexTo(it)
      this.visuals = visuals.copy { this.icon = icon.copy { this.palette = it } }
    }

    if (femIcon.path.isNotEmpty()) {
      indexImage(femIcon)
      findCorrectPalette(femIcon)?.let {
        icon.reindexTo(it)
        this.visuals = visuals.copy { this.icon = icon.copy { this.palette = it } }
      }
    }
  }

  private fun Visuals.Icon.read() = ImageIO.read(File("../../graphics/pokemon/$path.png"))

  private fun Visuals.Icon.write(image: BufferedImage) =
    ImageIO.write(image, "png", File("../../graphics/pokemon/$path.png"))

  private fun createIconBufferedImage(colorModel: ColorModel): BufferedImage {
    val sampleModel = MultiPixelPackedSampleModel(DataBuffer.TYPE_BYTE, 32, 64, 4)

    val raster = Raster.createWritableRaster(sampleModel, null)

    return BufferedImage(colorModel, raster, colorModel.isAlphaPremultiplied, null)
  }

  private fun indexImage(icon: Visuals.Icon) {
    val iconImage = icon.read()

    val colorModel = iconImage.colorModel

    if (colorModel is IndexColorModel && colorModel.mapSize <= 16) return

    println(
      "Indexing ${icon.path}, color model is ${colorModel::class.simpleName}, map size is ${(colorModel as? IndexColorModel)?.mapSize}"
    )

    val colorSet = buildSet {
      for (x in 0..<32) {
        for (y in 0..<64) {
          add(Color(iconImage.getRGB(x, y)))
        }
      }
    }

    check(colorSet.size <= 16) { "Too many colors in icon: ${icon.path}" }

    val colorList = colorSet.toList()
    val paddedList = List(16) { colorList.getOrElse(it) { Color(0) } }

    val newColorModel =
      IndexColorModel(
        4,
        16,
        ByteArray(16) { paddedList[it].red.toByte() },
        ByteArray(16) { paddedList[it].green.toByte() },
        ByteArray(16) { paddedList[it].blue.toByte() },
      )

    val newIcon = createIconBufferedImage(newColorModel)

    for (x in 0..<32) {
      for (y in 0..<64) {
        newIcon.setRGB(x, y, iconImage.getRGB(x, y))
      }
    }

    val testColorModel = newIcon.colorModel as IndexColorModel

    icon.write(newIcon)
  }

  private infix fun Color.box(other: Color) =
    equalize().zip(other.equalize()).maxOf { abs(it.first - it.second) }

  private fun Color.equalize() =
    listOf(red, blue, green).map { (it.toDouble() / 255 * 31).roundToInt() }

  private fun findCorrectPalette(icon: Visuals.Icon): Int? {
    val iconImage = icon.read()
    val colorModel = iconImage.colorModel as IndexColorModel

    val usedColors = buildSet {
      for (x in 0..<32) {
        for (y in 0..<64) {
          @Suppress("USELESS_CAST") add(iconImage.raster.getPixel(x, y, null as? IntArray).first())
        }
      }
    }

    val paletteColors = usedColors.map { Color(colorModel.getRGB(it)) }.drop(1)
    val assignedColors = usedColors.map { PALETTE_MAP[icon.palette]!![it] }.drop(1)

    if (paletteColors.zip(assignedColors).all { it.first box it.second == 0 }) return null

    val bestPalette =
      PALETTE_MAP.mapValues {
          paletteColors.sumOf { color -> it.value.drop(1).minOf { palColor -> palColor box color } }
        }
        .minBy { it.value }

    println(
      "Reindexing ${icon.path} from ${icon.palette} to ${bestPalette.key}. Box sum is ${bestPalette.value}"
    )

    return bestPalette.key
  }

  private fun Visuals.Icon.reindexTo(palette: Int) {
    val image = read()
    val newColorModel =
      IndexColorModel(
        4,
        16,
        PALETTE_MAP[palette]!!.map { it.red.toByte() }.toByteArray(),
        PALETTE_MAP[palette]!!.map { it.green.toByte() }.toByteArray(),
        PALETTE_MAP[palette]!!.map { it.blue.toByte() }.toByteArray(),
      )
    val newImage = createIconBufferedImage(newColorModel)
    for (x in 0..<32) {
      for (y in 0..<64) {
        @Suppress("USELESS_CAST")
        val pixelValue = image.raster.getPixel(x, y, null as? IntArray).first()
        if (pixelValue == 0) continue
        newImage.setRGB(x, y, image.getRGB(x, y))
      }
    }
    write(newImage)
  }
}

private val PALETTE_MAP =
  mapOf(
    0 to
      listOf(
        Color(98, 156, 131),
        Color(131, 131, 115),
        Color(189, 189, 189),
        Color(255, 255, 255),
        Color(189, 164, 65),
        Color(246, 246, 41),
        Color(213, 98, 65),
        Color(246, 148, 41),
        Color(139, 123, 255),
        Color(98, 74, 205),
        Color(238, 115, 156),
        Color(255, 180, 164),
        Color(164, 197, 255),
        Color(106, 172, 156),
        Color(98, 98, 90),
        Color(65, 65, 65),
      ),
    1 to
      listOf(
        Color(98, 156, 131),
        Color(115, 115, 115),
        Color(189, 189, 189),
        Color(255, 255, 255),
        Color(123, 156, 74),
        Color(156, 205, 74),
        Color(148, 246, 74),
        Color(238, 115, 156),
        Color(246, 148, 246),
        Color(189, 164, 90),
        Color(246, 230, 41),
        Color(246, 246, 172),
        Color(213, 213, 106),
        Color(230, 74, 41),
        Color(98, 98, 90),
        Color(65, 65, 65),
      ),
    2 to
      listOf(
        Color(98, 156, 131),
        Color(123, 123, 123),
        Color(189, 189, 180),
        Color(255, 255, 255),
        Color(115, 115, 205),
        Color(164, 172, 246),
        Color(180, 131, 90),
        Color(238, 197, 139),
        Color(197, 172, 41),
        Color(246, 246, 41),
        Color(246, 98, 82),
        Color(148, 123, 205),
        Color(197, 164, 205),
        Color(189, 41, 156),
        Color(98, 98, 90),
        Color(65, 65, 65),
      ),
    3 to
      listOf(
        Color(98, 156, 131),
        Color(115, 115, 115),
        Color(189, 189, 189),
        Color(255, 255, 255),
        Color(65, 106, 148),
        Color(98, 148, 164),
        Color(164, 197, 255),
        Color(238, 115, 156),
        Color(213, 98, 65),
        Color(189, 164, 90),
        Color(246, 230, 41),
        Color(246, 246, 172),
        Color(213, 213, 106),
        Color(246, 148, 41),
        Color(98, 98, 90),
        Color(65, 65, 65),
      ),
    4 to
      listOf(
        Color(98, 156, 131),
        Color(115, 115, 115),
        Color(189, 189, 189),
        Color(255, 255, 255),
        Color(123, 156, 74),
        Color(156, 205, 74),
        Color(65, 106, 148),
        Color(238, 115, 156),
        Color(246, 148, 246),
        Color(189, 164, 90),
        Color(246, 246, 139),
        Color(164, 197, 255),
        Color(98, 148, 164),
        Color(213, 98, 65),
        Color(98, 98, 90),
        Color(65, 65, 65),
      ),
    5 to
      listOf(
        Color(98, 156, 131),
        Color(123, 123, 123),
        Color(189, 189, 180),
        Color(255, 255, 255),
        Color(123, 156, 74),
        Color(156, 205, 74),
        Color(180, 131, 90),
        Color(238, 197, 139),
        Color(197, 172, 41),
        Color(246, 246, 41),
        Color(213, 98, 65),
        Color(148, 123, 205),
        Color(197, 164, 205),
        Color(246, 148, 41),
        Color(98, 98, 90),
        Color(65, 65, 65),
      ),
  )

private val PALETTE_FILTER =
  PALETTE_MAP.flatMap { (id, palette) -> palette.mapIndexed { color, idx -> id to (color to idx) } }
    .groupBy({ it.second }, { it.first })
