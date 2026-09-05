# Contributing to OpenZone Factions

Thanks for wanting to help. A few things to know before you open a pull request.

## Licence and rights

OpenZone is licensed under **CC BY-NC-SA 4.0** with an additional permission (see
`LICENSE` and `NOTICE`).

By submitting a contribution you agree that:

1. The contribution is your own work, or you have the right to submit it.
2. You assign copyright in the contribution to the project owner, or, where your
   jurisdiction does not permit assignment, you grant the owner an irrevocable,
   worldwide, royalty-free licence to use, modify, sublicense and relicense it,
   including under terms different from CC BY-NC-SA 4.0.

Point 2 exists so the project can be relicensed later without hunting down every
past contributor. Without it, a single unreachable contributor can freeze the
licence forever.

Contributions that carry code under a licence incompatible with CC BY-NC-SA 4.0
cannot be accepted. **GPL code in particular cannot go in.**

## Ground rules

- **Do not invent DayZ API.** Every engine call must be checked against the
  unpacked game scripts. If you are not sure, unpack the PBO and look.
- **No text in code.** Every user-facing string goes through the stringtable.
  `original` is Ukrainian, `english` is English, the twelve remaining vanilla
  columns repeat the Ukrainian original; the capital Ukrainian I is stored as
  Latin I because no Metron font of the game draws U+0406. The one exception is
  the VPP admin console: it is English in place and stays that way.
- **Core answers the questions, this mod supplies the answers.** Anything the
  PDA or VPP needs to know about a player's affiliation goes through Core's
  `OZ_Identity` service; do not reach into their screens from here except
  through the two glue PBOs.
- **Everything that knows about two mods lives in a glue PBO** (`_PDA`, `_VPP`),
  never in `OpenZone_Factions` itself.
- **Generated layouts are never hand-edited.** Edit the description under
  `ui/` and regenerate with `layout_build` of
  [dayz-agentic-modding-mcp](https://github.com/covalschi/dayz-agentic-modding-mcp);
  the gallery is looked at in two sizes and in both languages.
- **Never identify an item by inheritance from our own class.** Item classnames
  come from JSON so that admins can point the mod at items from any mod.
- Repository languages are English and Ukrainian only.

## Before you open a pull request

- The mod compiles: server boot and client compile check both clean.
- No new warnings in the server log.
- If you added a config field, it has a default, a migration, and a line in
  `Validate()`.
