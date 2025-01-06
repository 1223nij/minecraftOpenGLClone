#include <string_view>
#include <glm/vec2.hpp>

class Block {
public:
    std::string_view name;
    std::string_view id;
    glm::vec2 textureOffsets[6];

    constexpr Block(std::string_view n, std::string_view i,
                    glm::vec2 front, glm::vec2 back,
                    glm::vec2 left, glm::vec2 right,
                    glm::vec2 top, glm::vec2 bottom)
        : name(n), id(i), textureOffsets{front, back, left, right, top, bottom} {}

    constexpr bool operator==(const Block& other) const {
        return id == other.id;
    }
};

struct Blocks {
    static constexpr Block AIR{
        "Air", "minecraft:air",
        glm::vec2{-1, -1}, glm::vec2{-1, -1},
        glm::vec2{-1, -1}, glm::vec2{-1, -1},
        glm::vec2{-1, -1}, glm::vec2{-1, -1}
    };
    static constexpr Block DIRT{
        "Dirt", "minecraft:dirt",
        glm::vec2{2, 0}, glm::vec2{2, 0},
        glm::vec2{2, 0}, glm::vec2{2, 0},
        glm::vec2{2, 0}, glm::vec2{2, 0}
    };
    static constexpr Block STONE{
        "Stone", "minecraft:stone",
        glm::vec2{1, 0}, glm::vec2{1, 0},
        glm::vec2{1, 0}, glm::vec2{1, 0},
        glm::vec2{1, 0}, glm::vec2{1, 0}
    };
    static constexpr Block GRASS_BLOCK{
        "Stone", "minecraft:grass_block",
        glm::vec2{3, 0}, glm::vec2{3, 0},
        glm::vec2{3, 0}, glm::vec2{3, 0},
        glm::vec2{0, 0}, glm::vec2{3, 0}
    };
};
