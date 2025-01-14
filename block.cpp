#include <string_view>
#include <glm/vec2.hpp>

class Block {
public:
    std::string_view name;
    std::string_view id;
    glm::vec2 textureOffsets[6];
    glm::vec2 textureOffsetOverlays[6];
    bool isTransparent = false;

    constexpr Block(std::string_view n, std::string_view i,
                    glm::vec2 front, glm::vec2 back,
                    glm::vec2 left, glm::vec2 right,
                    glm::vec2 top, glm::vec2 bottom,
                    glm::vec2 frontOverlay, glm::vec2 backOverlay,
                    glm::vec2 leftOverlay, glm::vec2 rightOverlay,
                    glm::vec2 topOverlay, glm::vec2 bottomOverlay,
                    bool transparent)
        : name(n), id(i), textureOffsets{front, back, left, right, top, bottom}, textureOffsetOverlays{frontOverlay, backOverlay, leftOverlay, rightOverlay, topOverlay, bottomOverlay}, isTransparent(transparent){}

    constexpr bool operator==(const Block& other) const {
        return id == other.id;
    }
};

struct Blocks {
    static constexpr Block AIR{
        "Air", "minecraft:air",
        glm::vec2{4, -11}, glm::vec2{4, -11},
        glm::vec2{4, -11}, glm::vec2{4, -11},
        glm::vec2{4, -11}, glm::vec2{4, -11},
        glm::vec2{4, -11}, glm::vec2{4, -11},
        glm::vec2{4, -11}, glm::vec2{4, -11},
        glm::vec2{4, -11}, glm::vec2{4, -11},
        true
    };
    static constexpr Block DIRT{
        "Dirt", "minecraft:dirt",
        glm::vec2{2, 0}, glm::vec2{2, 0},
        glm::vec2{2, 0}, glm::vec2{2, 0},
        glm::vec2{2, 0}, glm::vec2{2, 0},
        AIR.textureOffsetOverlays[0], AIR.textureOffsetOverlays[0],
        AIR.textureOffsetOverlays[0], AIR.textureOffsetOverlays[0],
        AIR.textureOffsetOverlays[0], AIR.textureOffsetOverlays[0],
        false
    };
    static constexpr Block STONE{
        "Stone", "minecraft:stone",
        glm::vec2{1, 0}, glm::vec2{1, 0},
        glm::vec2{1, 0}, glm::vec2{1, 0},
        glm::vec2{1, 0}, glm::vec2{1, 0},
        AIR.textureOffsetOverlays[0], AIR.textureOffsetOverlays[0],
        AIR.textureOffsetOverlays[0], AIR.textureOffsetOverlays[0],
        AIR.textureOffsetOverlays[0], AIR.textureOffsetOverlays[0],
        false
    };
    static constexpr Block GRASS_BLOCK{
        "Grass Block", "minecraft:grass_block",
        glm::vec2{3, 0}, glm::vec2{3, 0},
        glm::vec2{3, 0}, glm::vec2{3, 0},
        glm::vec2{0, 0}, glm::vec2{2, 0},
        glm::vec2{6, -2}, glm::vec2{6, -2},
        glm::vec2{6, -2}, glm::vec2{6, -2},
        glm::vec2{0, 0}, AIR.textureOffsetOverlays[0],
        false
    };
    static constexpr Block OAK_PLANKS{
        "Oak Planks", "minecraft:oak_planks",
        glm::vec2{4, 0}, glm::vec2{4, 0},
        glm::vec2{4, 0}, glm::vec2{4, 0},
        glm::vec2{4, 0}, glm::vec2{4, 0},
        AIR.textureOffsetOverlays[0], AIR.textureOffsetOverlays[0],
        AIR.textureOffsetOverlays[0], AIR.textureOffsetOverlays[0],
        AIR.textureOffsetOverlays[0], AIR.textureOffsetOverlays[0],
        false
    };
    static constexpr Block OAK_LOG{
        "Oak Log", "minecraft:oak_log",
        glm::vec2{4, -1}, glm::vec2{4, -1},
        glm::vec2{4, -1}, glm::vec2{4, -1},
        glm::vec2{5, -1}, glm::vec2{5, -1},
        AIR.textureOffsetOverlays[0], AIR.textureOffsetOverlays[0],
        AIR.textureOffsetOverlays[0], AIR.textureOffsetOverlays[0],
        AIR.textureOffsetOverlays[0], AIR.textureOffsetOverlays[0],
        false
    };
    static constexpr Block OAK_LEAVES{
        "Oak Leaves", "minecraft:OAK_LEAVES",
        glm::vec2{4, -3}, glm::vec2{4, -3},
        glm::vec2{4, -3}, glm::vec2{4, -3},
        glm::vec2{4, -3}, glm::vec2{4, -3},
        glm::vec2{4, -3}, glm::vec2{4, -3},
        glm::vec2{4, -3}, glm::vec2{4, -3},
        glm::vec2{4, -3}, glm::vec2{4, -3},
        true
    };
};
