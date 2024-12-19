{ config, lib, pkgs, ... }:

with lib;

let
    cfg = config.xsession.windowManager.YATwm;
in

{
    options.xsession.windowManager.YATwm = {
        enable = mkEnableOption "YATwm";
        package = mkPackageOption pkgs null { };
    };

    config = mkIf cfg.enable {
        home.packages = [ cfg.package ];
        xsession.windowManager.command = "${cfg.package}/bin/YATwm";
        xsession.enable = true;
    };
}
