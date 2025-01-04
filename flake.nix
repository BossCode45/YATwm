{
    description = "YATwm";
    inputs = {
        nixpkgs.url = "github:NixOS/nixpkgs/nixos-24.11";
        libCommands = {
            url = "github:BossCode45/commands";
            inputs.nixpkgs.follows = "nixpkgs";
        };
    };
    outputs = { self, nixpkgs, ... }@inputs:
        let pkgs = nixpkgs.legacyPackages.x86_64-linux;
        in
          {
              devShells.x86_64-linux.default = pkgs.mkShell {
                  nativeBuildInputs = [
                      pkgs.gcc
                      pkgs.gnumake
                      pkgs.xorg.libX11
                      pkgs.xorg.libXrandr
                      pkgs.libnotify
                      pkgs.pkg-config
                      pkgs.clang-tools
                      inputs.libCommands.packages.x86_64-linux.default
                  ];
              };
              packages.x86_64-linux.YATwm =  (pkgs.callPackage ./YATwm.nix {inherit inputs;});
              packages.x86_64-linux.default = self.packages.x86_64-linux.YATwm;
              nixosModules.YATwm = import ./nix/module.nix;
              nixosModules.default = self.nixosModules.YATwm;
              homeManagerModules.YATwm = import ./nix/hm-module.nix;
              homeManagerModules.default = self.homeManagerModules.YATwm;
          };
}
