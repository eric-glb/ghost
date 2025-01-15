# 👻 spinning

This (🚀💥 hyper optimized) C program displays the animated ghost from the [ghostty](https://ghostty.dev) homepage in your terminal.

## Requirements

- Terminal size: at least 115x56
- C compiler (e.g., GCC, C99+)

![demo](/.github/demo.gif)

<details>
  <summary>Using with Nix</summary>
  
  
  ### Using Flakes
  
  1. **Clone the repository:**
  
     ```sh
     git clone https://github.com/themackabu/ghost.git
     cd ghost
     ```
  
  2. **Build the `ghost` package using Flakes:**
  
     ```sh
     nix build .#ghost
     ```
  
     This will build the `ghost` package and place the result in the `./result` directory.
  
  3. **Run the `ghost` binary:**
  
     ```sh
     ./result/bin/ghost
     ```
  
  ### Using Nix Configuration
  
  To include the `ghost` package in your NixOS configuration, you can add it to your `configuration.nix` file.
  
  1. **Add the Flake input to your `configuration.nix`:**
  
     ```nix
     {
       inputs = {
         nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
         flake-utils.url = "github:numtide/flake-utils";
         ghost.url = "path:./path/to/your/ghost/repo";
       };
  
       outputs = { self, nixpkgs, flake-utils, ghost }: {
         nixosConfigurations = {
           hostname = nixpkgs.lib.nixosSystem {
             system = "x86_64-linux";
             modules = [
               ./hardware-configuration.nix
               {
                 imports = [ ghost.nixosModules.default ];
  
                 environment.systemPackages = with pkgs; [
                   ghost.packages.x86_64-linux.ghost
                 ];
               }
             ];
           };
         };
       };
     }
     ```
  
  2. **Rebuild your NixOS system:**
  
     ```sh
     sudo nixos-rebuild switch --flake .#hostname
     ```
  
  ### Using Nix Profile with GitHub
  
  1. **Install the `ghost` package into your profile directly from GitHub:**
  
     ```sh
     nix profile install github:themackabu/ghost#ghost
     ```
  
  2. **Run the `ghost` binary:**
  
     ```sh
     ghost
     ```

</details>
