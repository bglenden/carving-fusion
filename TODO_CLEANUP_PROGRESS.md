# Item #2: TODO/FIXME Cleanup Progress

**Status**: In Progress

## TODO Inventory (21 found in src/)

### High Priority (Must Fix)
1. ❌ **src/core/PluginInitializer.cpp:11** - "TODO(dev): Fix HelloWorldCommand API issues"
   - Location: Commented out HelloWorldCommand include
   - Action: Either implement fully or remove dead code

2. ❌ **src/utils/ErrorHandler.cpp:38,53** - "TODO(dev): Show message to user via UI"
   - Action: Add UI integration for error display

3. ❌ **src/adapters/FusionLogger.cpp:208,219,250** - UI dialog stubs
   - Action: Complete confirmation dialog implementation

### Low Priority (Cleanup)
4. ❌ **src/core/PluginManagerUtils.cpp:19,25** - Placeholder comments
5. ❌ **src/adapters/FusionWorkspaceProfile.cpp:34** - Debug code
6. ❌ **src/commands/PluginCommandsCreation.cpp:82** - Preview functionality stub
7. ❌ **src/commands/PluginCommandsCreation.cpp:233,238** - Error logging
8. ❌ **src/core/PluginManagerPathsVisualization.cpp:68,73,133,160** - Placeholders
9. ❌ **src/commands/PluginCommandsExecution.cpp:59,71,74** - Error handling
10. ❌ **src/adapters/FusionLogger.cpp:136,265** - Backup logic, UI updates

### Ignored (Backup files)
- src/core/PluginInitializer.cpp.backup - Not in src/
- src/core/PluginManager.cpp.backup - Not in src/

## Progress Tracker
- [ ] High Priority TODOs (3 items)
- [ ] Low Priority TODOs (7 items with multiple instances)
- [ ] Verify: `make lint` shows zero TODO/FIXME/XXX
- [ ] Verify: All 287 tests still pass

**Note**: Most TODOs appear to be leftover cruft from development iterations.
