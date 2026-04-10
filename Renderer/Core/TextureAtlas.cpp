#include "TextureAtlas.h"
#include "Packing.h"
#include "Utils.h"

namespace Gfx
{
    uint8_t const k_assumedChannels = 4;
    uint8_t const k_debugColor[k_assumedChannels] = {36, 239, 20, 255}; 

    namespace Detail
    {
        uint8_t GetId(){
            static uint8_t id = std::numeric_limits<uint8_t>::max();
            id += 1;
            return id;
        }
    }

    TextureAtlas::TextureAtlas(uint32_t width, uint32_t height, std::string const& label)
    : _packer(new SkylinePacker(width, height))
    , _label(label)
    , _id(Detail::GetId())
    {
        _atlas.height = height;
		_atlas.width = width;
		_atlas.channelDepthBytes = 1; // single byte channel assumed
		_atlas.numChannels = k_assumedChannels; //channel depth of 4 assumed. RGBA8
		_atlas.data.resize(_atlas.SizeBytes());
		_atlas.label = label + "_Resource";

        Utils::FastFill(_atlas.data.data(), (uint32_t)_atlas.data.size(), 
            reinterpret_cast<std::byte const*>(&k_debugColor[0]), sizeof(k_debugColor));

    }

    //Packs animationSet into atlas and updates animation info
    //Returns false if pack was unsuccessful
    bool TextureAtlas::AddToAtlas(AnimationSet& animationSet, std::vector<TextureResource> const& textures)
    {
        assert(!IsPackingCompleted());

        //go through each animation
        uint32_t index = 0;
        for(auto& anim : animationSet.animations)
        {
            //pack into atlas
            auto oPackRes = _packer->Pack(textures[index].width, anim.frameRes.height);

            if(!oPackRes){
                return false;
            }

            RectPackResult res = *oPackRes;
            Rect sourcePos {Vec2u{0,0},Vec2u{textures[index].width, anim.frameRes.height}};
            //copy animation to texture
            Utils::Blit(
                _atlas.data.data(),
                Vec2u{res.x, res.y},
                _atlas.width,
                textures[index].data.data(),
                textures[index].width,
                sourcePos,
                _atlas.numChannels * _atlas.channelDepthBytes
            );

            //update animation location
            anim.startX = res.x;
            anim.startY = res.y;

            index += 1;
        }

        animationSet.atlasId = _id;

        return true;
    }

    void TextureAtlas::CompletePacking()
    {
        if(!IsPackingCompleted())
        {
            _packer.reset();
        }
    }
}