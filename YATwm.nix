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
        hash = "sha256-yQoyXGJE8JrSon/P5uhyN1rRwBH/kz0LCGIly3yNDhg=";
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
    ];
}
