// tacoRender (c) Nikolas Wipper 2024

#ifndef TR_GENERATORS_H
#define TR_GENERATORS_H

#include "tacoRender.h"

#ifdef __cplusplus
extern "C" {
#endif

TextureCubemap PrefilterCubemap(TextureCubemap cubemap);
TextureCubemap IrradianceCubemap(TextureCubemap cubemap);

#ifdef __cplusplus
}
#endif

#endif //TR_GENERATORS_H
