#ifndef TERRAFORM_TASK_SYSTEM_H__
#define TERRAFORM_TASK_SYSTEM_H__

namespace Unit {

class TerraformTaskSystem {
public:
    static void Init();
    static void Update();
    static void Shutdown();
};

}

#endif