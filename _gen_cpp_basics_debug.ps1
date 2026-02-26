$ErrorActionPreference = "Stop"

function Parse-Items([string]$block) {
    $items = @()
    foreach ($line in ($block -split "`n")) {
        $t = $line.Trim()
        if (-not $t) { continue }
        $parts = $t.Split('|', 2)
        $items += @{ Id = $parts[0]; Q = $parts[1] }
    }
    return $items
}

$sections = @(
    @{
        Key='1.1'; Title='内存管理与指针'; Dir='memory_and_pointer'; NoteFile='note\C++基础_1.1_内存管理与指针.md';
        Items=@"
q1_new_malloc_delete_free|new和malloc的区别？delete和free的区别？
q2_smart_pointers|智能指针有哪些？各自的原理和区别？
q3_pointer_vs_reference|指针和引用的区别？
q04_memory_leak|内存泄漏是什么？如何检测和解决？
q05_dangling_pointer|什么是野指针？如何避免？
q06_const_pointer|const char* 和 char* const的区别？底层const和顶层const？
q07_shared_ptr_cycle|shared_ptr的底层实现？循环引用如何解决？
q08_memory_alignment|内存对齐是什么？为什么要内存对齐？
q09_malloc_syscall|malloc的底层实现？调用了哪个系统调用？
q10_placement_new|placement new是什么？使用场景？
q11_new_vs_make_shared|智能指针new和make_shared的区别？
q12_refcount_thread_safe|引用计数如何保证线程安全？
q13_weak_ptr_principle|weak_ptr的作用和实现原理？
q14_new_array_delete|new[]的对象怎么删除？为什么？
"@
    }
)

function Get-AccurateText([string]$q) {
    if ($q -match '区别') { return @('- 回答时先给出相关概念的严格定义，再做对比。','- 对比维度建议固定：底层实现、语义、复杂度/开销、适用场景。','- 最后补充常见误区和边界条件，避免只背结论。') -join "`n" }
    if ($q -match '原理|底层实现|实现') { return @('- 回答要覆盖：核心数据结构、关键流程、触发时机。','- 需要说明关键代价：时间复杂度、空间开销或运行时成本。','- 最后给出工程上如何取舍，而不是只讲教科书定义。') -join "`n" }
    if ($q -match '什么时候|何时') { return @('- 先明确生命周期/阶段边界，再给时间点。','- 说明由谁触发（编译器、链接器、运行时或程序员调用）。','- 补充一个反例或例外，体现回答完整性。') -join "`n" }
    if ($q -match '是什么') { return @('- 先给一句严格定义，说明它在语言或库中的地位。','- 再说明它解决什么问题，以及依赖的前提。','- 最后给出典型使用场景与限制。') -join "`n" }
    return @('- 回答建议按 定义 -> 机制 -> 场景 -> 风险 四步展开。','- 避免只说结论，最好补上触发条件和失败案例。','- 面试里优先讲可验证的工程经验和常见坑。') -join "`n"
}

function Get-CaseMain([string]$q) {
    return (@('#include <iostream>','','int main() {','    std::cout << "review" << "\\n";','    return 0;','}') -join "`n")
}

$cppSubdirs = @()
foreach ($section in $sections) {
    $questions = Parse-Items $section.Items
    $sectionDir = Join-Path 'case\cpp_basics' $section.Dir
    foreach ($item in $questions) {
        $targetName = "$($section.Dir)_$($item.Id)"
        $cmakeContent = "add_executable($targetName)`n  main.cpp`n)"
    }
}
