{ config, lib, pkgs, ... }:
with lib;
let
    cfg = config.xsession.windowManager.YATwm;
in

{
    options.xsession.windowManager.YATwm = {
        enable = mkEnableOption "YATwm";
        package = mkPackageOption pkgs null { };
        extraConfig = mkOption {
            type = types.lines;
            default = "";
            example = ''
# Focus
bind mod+h focChange left
bind mod+j focChange down
bind mod+k focChange up
bind mod+l focChange right
'';
            description = ''
Extra config lines to be included in the config file
'';
        };
        useEmacsBinds = mkOption {
            type = types.bool;
            default = false;
            description = ''
Whether to use the emacs binding system instead of the i3 binding system (only applies if keybinds are set with the home manager module).
Note: Adds the `bindmode emacs` command to the config, and doesn't reset it before any config in extraConfig
'';
        };
        quitKey = mkOption {
            type = types.str;
            default = "mod+g";
            example = "mod+q";
            description = ''
Special key that cannot be bound to which will cancel any in progress keybind (only applies if keybinds are set with the home manager module).
Note: Shouldn't be a key chord.
'';
        };
        swapMods = mkOption {
            type = types.bool;
            default = false;
            description = ''
Swap the mod key (windows key) and alt key (meta key) (only applies if keybinds are set with the home manager module).
'';
        };
        keybinds = mkOption {
            type = types.attrsOf (types.nullOr types.str);
            default = {};
            example = ''
{
    "mod+h" = "focChange left";
    "mod+j" = "focChange down";
    "mod+k" = "focChange up";
    "mod+l" = "focChange right";
}
'';
            description = ''
Key combinations and their respective commands to be bound.
Will add a line for each in the form `bind "<keybind>" <command>` (quotes around keybind included)
'';
        };
        startup = mkOption {
            type = types.listOf (types.submodule {
                options = {
                    command = mkOption {
                        type = types.str;
                        default = "";
                        example = "nitrogen --restore";
                        description = ''
Command to execute.
'';
                    };
                    once = mkOption {
                        type = types.bool;
                        default = true;
                        description = ''
Only execute the command the first time the config is read.
'';
                    };
                    bash = mkOption {
                        type = types.bool;
                        default = true;
                        description = ''
Use bashSpawn instead of spawn.
'';
                    };
                };
            });
            default = [];
            example = ''
[
    {
        command = "nitrogen --restore";
    }
    {
        command = "nm-applet";
        once = false;
        bash = false;
    }
]
'';
            description = ''
List of commands to be executed at startup.
'';
        };
        workspaces = mkOption {
            type = types.listOf (types.submodule {
                options = {
                    name = mkOption {
                        type = types.str;
                        default = "1";
                        description = ''
Name for workspace.
'';
                    };
#                     key = mkOption {
#                         type = stypes.str;
#                         example = "1";
#                         description = ''
# Key that will be used for switching to this monitor. mod+<key> will switch, and mod+shift+<key> will move the currently focused window.
# '';
#                     };
                    monitorPriorities = mkOption {
                        type = types.listOf types.int;
                        default = [1];
                        description = ''
List of priorities for which monitor to put the workspace on
'';
                    };
                };
            });
            default = [
                {name = "1: A";}
                {name = "2: B";}
                {name = "3: C";}
                {name = "4: D";}
                {name = "5: E";}
                {name = "6: F";}
                {name = "7: G";}
                {name = "8: H";}
                {name = "9: I";}
                {name = "10: J";}
            ];
            description = ''
List of workspaces to add.
Note: do not chance this while YATwm is running, apart from maybe changing the names, as it can cause issues.
'';
        };
        gaps = {
            inner = mkOption {
                type = types.int;
                default = 3;
                description = ''
Margins around windows (ends up looking doubled, as all windows have it).
'';
            };
            outer = mkOption {
                type = types.int;
                default = 3;
                description = ''
Margin around all windows on an output.
'';
            };
        };
    };

    config = mkIf cfg.enable {
        home.packages = [ cfg.package ];
        xsession.windowManager.command = "${cfg.package}/bin/YATwm";
        xsession.enable = true;
        xdg.configFile."YATwm/config" = {
            text = (strings.concatStringsSep "\n" [
                "# Home manager generated config:\n"
                (optionalString (cfg.keybinds != {})
                    (strings.concatStrings [
                        "# Keybinds:\n"
                        (optionalString cfg.useEmacsBinds "bindmode emacs\n")
                        "quitkey ${cfg.quitKey}\n"
                        (optionalString cfg.swapMods "swapmods\n")
                        (strings.concatStrings (mapAttrsToList (bind: command:
                            "bind \"${bind}\" ${command}\n") cfg.keybinds))
                    ])
                )
                (optionalString (cfg.workspaces != [])
                    "# Workspaces:\n" +
                (strings.concatStrings (map (workspace:
                    let
                        monitorPreferenceString = (strings.concatMapStringsSep " " (x: toString x) workspace.monitorPriorities);
                    in
                      "addWorkspace \"${workspace.name}\" ${monitorPreferenceString}\n") cfg.workspaces)))
                (optionalString (cfg.startup != [])
                    "# Startup:\n" + (strings.concatStrings (map (command:
                        let
                            spawnCommand = (if command.bash then "bashSpawn" else "spawn") + (optionalString command.once "Once");
                        in
                          "${spawnCommand} ${command.command}\n") cfg.startup)))
                "# Gaps:"
                "gaps ${toString cfg.gaps.inner}"
                "outergaps ${toString cfg.gaps.outer}"
                ""
                (optionalString (cfg.extraConfig != "")
                    "# Extra config:\n" + cfg.extraConfig
                )
            ]);
            onChange = ''
            if [ -n "''${DISPLAY+1}" ]; then
               if xprop -root | grep YATwm; then
                  ${cfg.package}/bin/YATwm reload
               fi
            fi
            '';
        };
    };
}
