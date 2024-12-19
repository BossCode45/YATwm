{ config, lib, pkgs, ... }:

with lib;

let
    cfg = config.services.xserver.windowManager.YATwm;
in

{
    options.services.xserver.windowManager.YATwm = {
        enable = mkEnableOption "YATwm";

        package = mkPackageOption pkgs null { };
    };

    config = mkIf cfg.enable {
        services.xserver.windowManager.session = [{
            name  = "YATwm";
            start = ''
        ${cfg.package}/bin/YATwm &
        waitPID=$!
      '';
        }];
        environment.systemPackages = [ cfg.package ];
    };
}
