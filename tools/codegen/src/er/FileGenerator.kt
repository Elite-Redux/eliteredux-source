package er

import er.abilities.AbilitiesEnumGenerator

object FileGenerator {
    @JvmStatic
    fun main(args: Array<String>) {
        AbilitiesEnumGenerator.generate(args.first())
    }
}