; ModuleID = 'test3.c'
source_filename = "test3.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @main() #0 {
entry:
  %retval = alloca i32, align 4
  %a1 = alloca i32, align 4
  %a2 = alloca i32, align 4
  %x = alloca i32, align 4
  %y = alloca i32, align 4
  %c = alloca i32, align 4
  store i32 0, i32* %retval, align 4
  %0 = load i32, i32* %a1, align 4
  store i32 %0, i32* %x, align 4
  %1 = load i32, i32* %a2, align 4
  store i32 %1, i32* %y, align 4
  store i32 0, i32* %c, align 4
  %2 = load i32, i32* %x, align 4
  %cmp = icmp eq i32 %2, 123
  br i1 %cmp, label %if.then, label %if.else

if.then:                                          ; preds = %entry
  store i32 3, i32* %c, align 4
  br label %if.end

if.else:                                          ; preds = %entry
  store i32 2, i32* %c, align 4
  br label %if.end

if.end:                                           ; preds = %if.else, %if.then
  %3 = load i32, i32* %y, align 4
  %cmp1 = icmp sgt i32 %3, 10
  br i1 %cmp1, label %if.then2, label %if.else3

if.then2:                                         ; preds = %if.end
  %4 = load i32, i32* %c, align 4
  %add = add nsw i32 %4, 100
  store i32 %add, i32* %c, align 4
  br label %if.end5

if.else3:                                         ; preds = %if.end
  %5 = load i32, i32* %c, align 4
  %add4 = add nsw i32 %5, 200
  store i32 %add4, i32* %c, align 4
  br label %if.end5

if.end5:                                          ; preds = %if.else3, %if.then2
  %6 = load i32, i32* %retval, align 4
  ret i32 %6
}

attributes #0 = { noinline nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 10.0.0-4ubuntu1~18.04.2 "}
