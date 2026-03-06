#include "Precompiled.h"
#include "LAnimReader.h"
#include "Core/OS/FileSystem.h"
#include "Graphics/Model.h"
#include "Graphics/Animation/Skeleton.h"
#include "Graphics/Animation/Animation.h"

#include <ozz/animation/runtime/skeleton.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/base/io/archive.h>
#include <ozz/base/io/stream.h>

namespace Lumos
{
    bool LAnimReader::Read(Arena* arena, const String8& path, Graphics::Model& outModel)
    {
        ArenaTemp scratch = ScratchBegin(&arena, 1);

        String8 physicalPath;
        if(!FileSystem::Get().ResolvePhysicalPath(scratch.arena, path, &physicalPath))
        {
            LERROR("LAnimReader: Failed to resolve path %.*s", (int)path.size, path.str);
            ScratchEnd(scratch);
            return false;
        }

        int64_t fileSize = FileSystem::GetFileSize(physicalPath);
        if(fileSize < (int64_t)sizeof(LAnimHeader))
        {
            LERROR("LAnimReader: File too small %.*s", (int)path.size, path.str);
            ScratchEnd(scratch);
            return false;
        }

        Arena* fileArena = ArenaAlloc((u64)fileSize + Kilobytes(4));
        u8* fileData = FileSystem::ReadFile(fileArena, physicalPath);
        if(!fileData)
        {
            LERROR("LAnimReader: Failed to read %.*s", (int)path.size, path.str);
            ArenaRelease(fileArena);
            ScratchEnd(scratch);
            return false;
        }

        const LAnimHeader* header = (const LAnimHeader*)fileData;
        if(memcmp(header->Magic, LANIM_MAGIC, 4) != 0)
        {
            LERROR("LAnimReader: Invalid magic in %.*s", (int)path.size, path.str);
            ArenaRelease(fileArena);
            ScratchEnd(scratch);
            return false;
        }

        if(header->Version != LANIM_VERSION)
        {
            LERROR("LAnimReader: Version mismatch (got %u, expected %u)", header->Version, LANIM_VERSION);
            ArenaRelease(fileArena);
            ScratchEnd(scratch);
            return false;
        }

        u8* ptr = fileData + sizeof(LAnimHeader);
        u8* fileEnd = fileData + fileSize;

        // Deserialize skeleton
        SharedPtr<Graphics::Skeleton> skeleton;
        if(header->SkeletonBlobSize > 0)
        {
            if(ptr + header->SkeletonBlobSize > fileEnd)
            {
                LERROR("LAnimReader: Skeleton blob truncated");
                ArenaRelease(fileArena);
                ScratchEnd(scratch);
                return false;
            }

            ozz::io::MemoryStream memStream;
            memStream.Write(ptr, header->SkeletonBlobSize);
            memStream.Seek(0, ozz::io::Stream::kSet);

            ozz::animation::Skeleton* ozzSkel = new ozz::animation::Skeleton();
            ozz::io::IArchive ia(&memStream);
            ia >> *ozzSkel;

            skeleton = CreateSharedPtr<Graphics::Skeleton>(ozzSkel);
            ptr += header->SkeletonBlobSize;
        }

        // Deserialize animations
        TDArray<SharedPtr<Graphics::Animation>> animations;
        for(u32 i = 0; i < header->AnimationCount; i++)
        {
            // Name
            if(ptr + 4 > fileEnd)
            {
                LERROR("LAnimReader: Animation %u truncated (nameLen)", i);
                ArenaRelease(fileArena);
                ScratchEnd(scratch);
                return false;
            }

            u32 nameLen;
            memcpy(&nameLen, ptr, 4); ptr += 4;

            if(nameLen > 4096 || ptr + nameLen + 1 > fileEnd)
            {
                LERROR("LAnimReader: Animation %u bad nameLen %u", i, nameLen);
                ArenaRelease(fileArena);
                ScratchEnd(scratch);
                return false;
            }

            std::string animName((const char*)ptr, nameLen);
            ptr += nameLen + 1;

            // Blob
            if(ptr + 4 > fileEnd)
            {
                LERROR("LAnimReader: Animation %u truncated (blobSize)", i);
                ArenaRelease(fileArena);
                ScratchEnd(scratch);
                return false;
            }

            u32 blobSize;
            memcpy(&blobSize, ptr, 4); ptr += 4;

            if(ptr + blobSize > fileEnd)
            {
                LERROR("LAnimReader: Animation %u blob truncated", i);
                ArenaRelease(fileArena);
                ScratchEnd(scratch);
                return false;
            }

            ozz::io::MemoryStream memStream;
            memStream.Write(ptr, blobSize);
            memStream.Seek(0, ozz::io::Stream::kSet);

            ozz::animation::Animation* ozzAnim = new ozz::animation::Animation();
            ozz::io::IArchive ia(&memStream);
            ia >> *ozzAnim;

            auto anim = CreateSharedPtr<Graphics::Animation>(animName, ozzAnim, skeleton);
            animations.PushBack(anim);
            ptr += blobSize;
        }

        // Read bind poses
        TDArray<Mat4> bindPoses;
        if(header->BindPoseCount > 0)
        {
            u32 bindPoseBytes = header->BindPoseCount * sizeof(Mat4);
            if(ptr + bindPoseBytes > fileEnd)
            {
                LERROR("LAnimReader: Bind poses truncated");
                ArenaRelease(fileArena);
                ScratchEnd(scratch);
                return false;
            }

            bindPoses.Resize(header->BindPoseCount);
            memcpy(bindPoses.Data(), ptr, bindPoseBytes);
            ptr += bindPoseBytes;
        }

        // Apply to model
        outModel.SetSkeleton(skeleton);
        outModel.SetAnimations(animations);
        outModel.SetBindPoses(bindPoses);

        LINFO("LAnimReader: loaded skeleton=%s anims=%u bindPoses=%u from %.*s",
              skeleton ? "yes" : "no", header->AnimationCount, header->BindPoseCount,
              (int)path.size, path.str);

        ArenaRelease(fileArena);
        ScratchEnd(scratch);
        return true;
    }
}
