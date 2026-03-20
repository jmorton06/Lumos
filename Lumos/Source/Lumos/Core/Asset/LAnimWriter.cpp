#include "Precompiled.h"
#include "LAnimWriter.h"
#include "Core/OS/FileSystem.h"
#include "Graphics/Animation/Skeleton.h"
#include "Graphics/Animation/Animation.h"

#include <ozz/animation/runtime/skeleton.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/base/io/archive.h>
#include <ozz/base/io/stream.h>

namespace Lumos
{
    bool LAnimWriter::Write(const String8& outputPath, const LAnimWriteData& data)
    {
        // Serialize skeleton to memory blob
        TDArray<u8> skeletonBlob;
        if(data.Skeleton && data.Skeleton->Valid())
        {
            ozz::io::MemoryStream memStream;
            ozz::io::OArchive oa(&memStream);
            oa << data.Skeleton->GetSkeleton();

            size_t blobSize = memStream.Size();
            skeletonBlob.Resize((u32)blobSize);
            memStream.Seek(0, ozz::io::Stream::kSet);
            memStream.Read(skeletonBlob.Data(), blobSize);
        }

        // Serialize each animation to memory blob
        struct AnimBlob
        {
            std::string Name;
            TDArray<u8> Data;
        };
        TDArray<AnimBlob> animBlobs;

        for(int i = 0; i < data.Animations.Size(); i++)
        {
            auto& anim = data.Animations[i];
            if(!anim)
                continue;

            AnimBlob blob;
            blob.Name = anim->GetName();

            ozz::io::MemoryStream memStream;
            ozz::io::OArchive oa(&memStream);
            oa << anim->GetAnimation();

            size_t blobSize = memStream.Size();
            blob.Data.Resize((u32)blobSize);
            memStream.Seek(0, ozz::io::Stream::kSet);
            memStream.Read(blob.Data.Data(), blobSize);

            animBlobs.PushBack(std::move(blob));
        }

        // Calculate total file size
        // Header
        u32 offset = sizeof(LAnimHeader);

        // Skeleton blob
        u32 skeletonBlobSize = (u32)skeletonBlob.Size();
        offset += skeletonBlobSize;

        // Animation blobs: each is [nameLen:u32][name:chars][blobSize:u32][blob:bytes]
        for(int i = 0; i < animBlobs.Size(); i++)
        {
            offset += 4; // nameLen
            offset += (u32)animBlobs[i].Name.size() + 1; // name + null
            offset += 4; // blobSize
            offset += (u32)animBlobs[i].Data.Size();
        }

        // Bind poses: raw Mat4 array
        u32 bindPoseCount = (u32)data.BindPoses.Size();
        offset += bindPoseCount * sizeof(Mat4);

        u32 totalSize = offset;

        // Build header
        LAnimHeader header = {};
        memcpy(header.Magic, LANIM_MAGIC, 4);
        header.Version          = LANIM_VERSION;
        header.AssetUUID        = data.AssetUUID;
        header.SkeletonBlobSize = skeletonBlobSize;
        header.AnimationCount   = (u32)animBlobs.Size();
        header.BindPoseCount    = bindPoseCount;
        header.TotalFileSize    = totalSize;
        header.Reserved[0]      = 0;
        header.Reserved[1]      = 0;

        // Write to buffer
        TDArray<u8> buffer(totalSize);
        buffer.Resize(totalSize);
        memset(buffer.Data(), 0, totalSize);

        u8* ptr = buffer.Data();

        // Header
        memcpy(ptr, &header, sizeof(LAnimHeader));
        ptr += sizeof(LAnimHeader);

        // Skeleton blob
        if(skeletonBlobSize > 0)
        {
            memcpy(ptr, skeletonBlob.Data(), skeletonBlobSize);
            ptr += skeletonBlobSize;
        }

        // Animation blobs
        for(int i = 0; i < animBlobs.Size(); i++)
        {
            auto& ab = animBlobs[i];

            u32 nameLen = (u32)ab.Name.size();
            memcpy(ptr, &nameLen, 4); ptr += 4;
            memcpy(ptr, ab.Name.c_str(), nameLen + 1); ptr += nameLen + 1;

            u32 blobSize = (u32)ab.Data.Size();
            memcpy(ptr, &blobSize, 4); ptr += 4;
            memcpy(ptr, ab.Data.Data(), blobSize); ptr += blobSize;
        }

        // Bind poses
        if(bindPoseCount > 0)
        {
            memcpy(ptr, data.BindPoses.Data(), bindPoseCount * sizeof(Mat4));
            ptr += bindPoseCount * sizeof(Mat4);
        }

        return FileSystem::WriteFile(outputPath, buffer.Data(), totalSize);
    }
}
