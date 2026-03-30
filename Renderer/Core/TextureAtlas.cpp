#include "TextureAtlas.h"
#include "Packing.h"
#include "Utils.h"

namespace Gfx
{
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
		_atlas.numChannels = (uint8_t)3; //channel depth of 3 assumed
		_atlas.data.resize(_atlas.SizeBytes());
		_atlas.label = label + "_Resource";

    }

    //Packs animationSet into atlas and updates animation info
    //Returns false if pack was unsuccessful
    bool TextureAtlas::AddToAtlas(AnimationSet& animationSet)
    {
        assert(!IsPackingCompleted());

        //go through each animation
        for(auto& anim : animationSet.animations)
        {
            //pack into atlas
            auto oPackRes = _packer->Pack(anim.texture.width, anim.frameRes.height);

            if(!oPackRes){
                return false;
            }

            RectPackResult res = *oPackRes;
            Rect sourcePos {Vec2u{0,0},Vec2u{anim.texture.width, anim.frameRes.height}};
            //copy animation to texture
            Utils::Blit(
                _atlas.data.data(),
                Vec2u{res.x, res.y},
                _atlas.width,
                anim.texture.data.data(),
                anim.texture.width,
                sourcePos,
                _atlas.numChannels + _atlas.channelDepthBytes
            );

            //update animation location
            anim.startX = res.x;
            anim.startY = res.y;
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