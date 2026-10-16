#ifndef SIMPLEMODELANIM_HPP
#define SIMPLEMODELANIM_HPP

class eModel;
class eSolid;
struct bMatrix4;

namespace SimpleModelAnim {

void Reset();
void Init();
void Update();
void Animate(eModel *model, eSolid *solid, bMatrix4 *local_world);

} // namespace SimpleModelAnim

#endif
