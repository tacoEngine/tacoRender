// tacoRender (c) Nikolas Wipper 2024-2025

/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

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
