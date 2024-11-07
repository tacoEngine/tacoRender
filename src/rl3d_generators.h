// rl3d (c) Nikolas Wipper 2024

#ifndef RL3D_GENERATORS_H
#define RL3D_GENERATORS_H

#include "rl3d.h"

#ifdef __cplusplus
extern "C" {
#endif

TextureCubemap PrefilterCubemap(TextureCubemap cubemap);
TextureCubemap IrradianceCubemap(TextureCubemap cubemap);

#ifdef __cplusplus
}
#endif

#endif //RL3D_GENERATORS_H
