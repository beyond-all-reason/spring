/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef TERRAFORM_TASK_SYSTEM_H__
#define TERRAFORM_TASK_SYSTEM_H__

namespace Unit {

class TerraformTaskSystem {
public:
    static void Init();
    static void Update();
    static void Shutdown();

    static void SendEvents();
};

}

#endif