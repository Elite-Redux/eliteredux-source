# Wiki System (Information Menu)

The Wiki (formerly "Information Menu", accessed via "Wiki" in the Start Menu) is Elite Redux's in-game wiki/help system that provides players with guides and documentation.

## File Structure

### Main Content File
`proto/HelpArticles.textproto` - This is where all Wiki content is edited

### Structure Format
```textproto
help_category {
  title: "Category Name"
  color: RED  # Options: DARK, BLUE, RED, YELLOW, GREEN
  help_article {
    title: "Article Title"
    page: "Page 1 content"
    page: "Page 2 content"  # Multiple pages supported
    page: "Page 3 content"
  }
  help_article {
    title: "Another Article"
    page: "Single page content"
  }
}
```

## Key Features

- **Categories**: Appear as colored tabs in the menu
- **Articles**: Each category can contain multiple articles
- **Pages**: Each article can have multiple pages (player navigates with L/R buttons)
- **Text Formatting**: 
  - Use `\n` for line breaks
  - Text automatically wraps to fit the window
  - Can concatenate strings with quotes for better formatting

## Example Entry
```textproto
help_category {
  title: "Game Mechanics"
  color: BLUE
  help_article {
    title: "Ability System"
    page: "Elite Redux features a unique 4-ability system:"
          "\n\n• 1 Changeable Ability (can be swapped)"
          "\n• 3 Innate Abilities (always active)"
    page: "Abilities can be changed using Ability Capsules"
          "\nor Ability Patches obtained from various NPCs."
  }
}
```

## Color Options
- `DARK` - Dark/black theme
- `BLUE` - Blue theme
- `RED` - Red theme (default)
- `YELLOW` - Yellow theme
- `GREEN` - Green theme

## Implementation Details

### Core Files
- **Content Definition**: `proto/HelpArticles.textproto`
- **Proto Schema**: `proto/HelpArticles.proto`
- **UI Logic**: `src/ui_information_menu.c`
- **Generated Header**: `include/generated/data/text/help_articles.h` (auto-generated, do not edit)

### Build Process
1. Edit `HelpArticles.textproto` with your content
2. The codegen tool automatically generates the C header file
3. Build the ROM to see changes in-game

### UI Behavior
- Players navigate categories with left/right
- Articles are selected with up/down
- Pages within articles are browsed with L/R buttons
- Maximum of 5 entries visible on screen at once (scrolls if more)

## Best Practices

1. **Keep titles concise** - They need to fit in the menu list
2. **Use multiple pages** - Break up long content across pages for readability
3. **Organize logically** - Group related topics in the same category
4. **Test in-game** - Some formatting may look different in-game vs in the file
5. **Use line breaks wisely** - The window has limited vertical space

## Adding New Content

To add a new help article:
1. Open `proto/HelpArticles.textproto`
2. Either add to an existing `help_category` or create a new one
3. Add your `help_article` with title and page(s)
4. Build the ROM to test

Remember: The generated header file is automatically created from the textproto file during the build process - never edit it directly.