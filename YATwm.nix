{
    stdenv,
    fetchgit,
    xorg,
    libnotify,
    pkg-config,
    inputs,
    ...
}:


stdenv.mkDerivation {
    pname = "YATwm";
    version = "0.0.1";

    src = fetchgit {
        url = "https://git.tehbox.org/cgit/boss/YATwm.git/";
        rev = "v0.0.1";
        hash = "sha256-A4Yra/903rOeEbXfFia/A2HRPrFyE1b05mzHWlDImCY=";
    };

    installPhase = ''
runHook preInstall
mkdir -p $out/bin
install -D -m 755 YATwm $out/bin/YATwm
install -D -m 644 yat.desktop $out/usr/share/xsessions/yat.desktop
install -D -m 644 config $out/etc/YATwm/config
runHook postInstall
    '';
    
    buildInputs = [
        xorg.libX11
        xorg.libXrandr
        libnotify
        pkg-config
        inputs.libCommands.packages.x86_64-linux.default
    ];
}
