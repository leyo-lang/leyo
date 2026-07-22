/*
 * Copyright (c) 2026 Leyo Contributors
 * SPDX-License-Identifier: MIT
 */

#ifndef WIZARD_H
#define WIZARD_H

#include "../include/errors.h"
#include "../include/codes.h"
#include "../include/codes.h"

LogConfig readLogConfig(void);
bool writeDefaultLyst(void);
bool runInitWizard(void);

#endif
