#pragma once

#include <memory>

namespace flexy {

// 两个缺点
// 1. _Tp 不能是final
// 2. _Tp 的构造函数不能是 private
// 优点
// _Tp的构造函数可以是protected
// template <typename _Tp, typename ..._Args> 
// std::shared_ptr<_Tp> make_shared(_Args &&...__args) {
//     struct EnableMakeShared : public _Tp {
//         EnableMakeShared(_Args&&... args) 
//             : _Tp(std::forward<_Args>(args)...)
//         {}
//     };
//     return std::static_pointer_cast<_Tp>(std::make_shared<EnableMakeShared>
//                                     (std::forward<_Args>(__args)...));
// }

}