{
  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";

  outputs = { self, nixpkgs }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs { inherit system; };
    in
    {
      devShells.${system}.default = (pkgs.buildFHSEnv {
        name = "snowlayer-conifer";

        targetPkgs = p: with p; [
          platformio-core

          # Flashing tools
          wchisp # factory-bootloader flasher
          openocd # wch-link programmer

          python3

          # Shared libs the PlatformIO-bundled binaries expect. These are
          # prebuilt FHS ELFs (pio ships its own openocd rather than using the
          # one above), so they dynamically link against the usual suspects.
          stdenv.cc.cc.lib
          zlib
          libusb1
          ncurses5
          systemd # libudev.so.1, needed by the bundled openocd
        ];

#        runScript = "bash";

        profile = ''
          export PLATFORMIO_CORE_DIR="$PWD/.platformio"
        '';
      }).env;
    };
}
