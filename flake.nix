{
    description = "A Nix-flake-based development environment";

    inputs = {
        nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    };

    outputs = { self , nixpkgs ,... }: let
        system = "x86_64-linux";
    in {
        devShells."${system}".default = let
            pkgs = import nixpkgs {
                inherit system;
            };
        in pkgs.mkShell {
            packages = with pkgs; [
                libcxx
                clang
                clang-tools
                gdb
                valgrind
                premake5
                fmt
                glfw
                vulkan-headers
                vulkan-tools
                vulkan-validation-layers
                vulkan-extension-layer
                vulkan-loader
                julia
                openspecfun
                rocmPackages.clr
                rocmPackages.hip-common
                glm
                gtest
                rubber
                python311Packages.pygments
                inotify-tools
            ];

            shellHook = ''
                export NIX_LD_LIBRARY_PATH="/opt/intel/oneapi/compiler/latest/lib:$NIX_LD_LIBRARY_PATH"
                exec zsh
                '';
            NVIDIA=1;
        };
    };
}
