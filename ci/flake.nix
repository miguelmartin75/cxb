{
  description = "cxb C++ library – Nix flake";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";

  outputs = { self, nixpkgs }: let
    systems = [
      "x86_64-linux"
      "aarch64-linux"
      "x86_64-darwin"
      "aarch64-darwin"
    ];
    forAllSystems = f: nixpkgs.lib.genAttrs systems (system: f system);
  in {
    devShells = forAllSystems (system: let
      pkgs = import nixpkgs { inherit system; };
    in {
      default = pkgs.mkShell {
        # Build tools & libraries required for cxb
        packages = with pkgs; [
          cmake
          ninja
          gcc
          clang
          clang-tools
          lld
          git
        ];

        shellHook = ''
          # Default to Clang – scripts can override with gcc
          export CC=clang
          export CXX=clang++
          echo "Entering cxb dev shell for ${system} (CC=$CC)"
        '';
      };  # default shell
    });
  };  # outputs
} 