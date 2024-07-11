#ifndef GUARD_FIELD_TASKS_H
#define GUARD_FIELD_TASKS_H

void SetUpFieldTasks(void);
void ActivatePerStepCallback(u8 callbackId);
void ResetFieldTasksArgs(void);
void SetSootopolisGymCrackedIceMetatiles(void);

// allows to efficiently redraw over a colunm of metatiles
struct UColumnCoords8 {
    u8 x; 
    u8 y0; //from, the start
    u8 y1; //to the end INCLUDED so one case is {x, y, y}
};


#endif // GUARD_FIELD_TASKS_H
