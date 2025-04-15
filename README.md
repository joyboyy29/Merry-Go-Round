# Hashify - Merry-Go-Round (MGR)

> An experimental 256-bit hash algorithm blending SHA-256’s structure with Whirlpool-like S-box diffusion and custom rotational mixing.

Merry-Go-Round (MGR) is a compact, single-header hashing library written in modern C++.  
It’s designed for fun, entropy exploration, and experimentation in the world of custom cryptographic primitives.

<div align="center">
                          
                             █                              
                             █                              
                             █                              
                             █                              
                             █                              
               █  █████      █                              
           ██ ████████████   █                              
              ██████████████ █                              
              █████████████  █          ███                 
              ██   █████████ █   ███████  ███████    █      
                    ██████████████████████  ████████        
                   ███████████████████████   ███████        
                   ███████████████████████                  
               ██████████████████████████                   
             ██   ██████████████   ███████                  
            ██    █          █      ██  ███                 
                   █         █     ██     ██                
                     █       █   ██       █                 
                             █  ██       ██                 
                             █          █                   
                             █                              
                             █                              
                             █                              
                             █                              
                                                                                     

---

<div align="left">
  
## Usage

```cpp
#include <iostream>
#include <numbers>

#include "hashify.hpp"

using mgr = Hashify::MerryGoRound;

int main() {
    auto input = mgr::to_bytes("the world is spinning");
    auto hashed = mgr::hash(input);

    // The next three outputs should all be the same
    std::cout << "Out 1: " << hashed << std::endl;

    std::string hash_str = mgr::to_hex_string(hashed);
    std::cout << "Out 2: " << hash_str << std::endl;

    std::cout << "Out 3: " << mgr::hash_to_hex(input) << std::endl;

    /* ------------------------------------------------------------*/

    // overload + SFINAE usage
    std::cout << "string literal test: " << mgr::hash("test") << std::endl;
    std::cout << "addition: " << mgr::hash(3 + 3) << std::endl;
    std::cout << "more addition: " << mgr::hash(4 + 2) << std::endl;
    std::cout << "pi!: " << mgr::hash(std::numbers::pi_v<double>) << std::endl;

    std::cout << std::numbers::pi_v<double> << std::endl;
    return 0;
}
```

## Disclaimer
This is not a cryptographically secure hash.
Use it for experimenting, or fun — not for security.
Your boss will get mad at you if you do.
