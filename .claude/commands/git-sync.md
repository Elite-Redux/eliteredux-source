Perform an intelligent git sync operation for the Elite Redux project. Use your AI capabilities to understand the changes and create meaningful commit messages.

Steps:
1. Check git status and analyze what has changed
2. Read the actual content of changed files to understand:
   - What was modified and why
   - The purpose and impact of the changes
   - Any patterns or relationships between changes
3. If there are uncommitted changes:
   - Stage all changes
   - Create an intelligent commit message that:
     * Explains WHAT changed in clear, specific terms
     * Explains WHY it changed (the purpose/goal)
     * Uses conventional commit format when appropriate (feat:, fix:, docs:, refactor:, etc.)
     * Is concise but descriptive
     * Reflects the actual nature of the changes, not just file names
4. Handle git operations:
   - Pull latest changes (regular merge, not rebase)
   - Resolve any conflicts intelligently if possible
   - Push to remote
5. Provide helpful feedback throughout

Examples of good AI-generated commit messages:
- "feat: Add Wiki entry to Littleroot tutorial NPC menu"
- "fix: Correct menu indices after inserting new tutorial option"
- "refactor: Reorganize trainer script tools into dedicated directory"
- "docs: Update CLAUDE.md with git sync workflow documentation"

Be thoughtful and contextual. Don't just list files - understand and explain the changes.