{
  description = "A flake for building and developing the ghost C CLI tool";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = {
    self,
    nixpkgs,
    flake-utils,
  }:
    flake-utils.lib.eachDefaultSystem (system: let
      pkgs = import nixpkgs {inherit system;};
    in {
      packages = {
        ghost = pkgs.stdenv.mkDerivation {
          pname = "ghost";
          version = "0.0.1";

          src = ./.;

          buildInputs = [pkgs.clang pkgs.libbsd];

          buildPhase = ''
            mkdir -p $out/bin
            ${pkgs.clang}/bin/clang -std=c99 -O3 -march=native -flto -ffast-math \
              -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_POSIX_C_SOURCE=200809L \
              src/ghost.c src/frames.c -o $out/bin/ghost
          '';

          installPhase = "true";

          meta = with pkgs.lib; {
            description = "A C CLI tool for terminal animations";
            license = licenses.mit;
            maintainers = ["theMackabu" "linuxmobile"];
            platforms = platforms.linux;
          };
        };
        default = self.packages.${system}.ghost;
      };

      devShell = pkgs.mkShell {
        buildInputs = [pkgs.clang pkgs.cmake pkgs.libbsd pkgs.maid];
      };
    });
}
