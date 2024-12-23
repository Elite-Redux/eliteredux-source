package er

import er.defines.AbilitiesEnumGenerator
import er.defines.MovesEnumGenerator
import er.defines.SpeciesEnumGenerator
import java.io.File

object FileGenerator {
    @JvmStatic
    fun main(args: Array<String>) {
        val (type, file) = args
        try {
            File(file).parentFile.mkdirs()
            when (type) {
                "abilities" -> AbilitiesEnumGenerator.generate(file)
                "moves" -> MovesEnumGenerator.generate(file)
                "species" -> SpeciesEnumGenerator.generate(file)
                else -> error("Invalid file type $type")
            }
        } catch (e: Exception) {
            throw Exception("Failed processing $type generating file $file", e)
        }
    }
}