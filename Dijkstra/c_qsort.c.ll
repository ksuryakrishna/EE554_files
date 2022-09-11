@SIZE = common global i32 0, align 4
@array = common global [1000000 x i32] zeroinitializer, align 16
  %1 = alloca i32, align 4
  %2 = alloca i32, align 4
  %3 = alloca i8**, align 8
  %STEP = alloca i32, align 4
  store i32 0, i32* %1, align 4
  store i32 %argc, i32* %2, align 4
  store i8** %argv, i8*** %3, align 8
  %4 = load i8**, i8*** %3, align 8
  %5 = getelementptr inbounds i8*, i8** %4, i64 1
  %6 = load i8*, i8** %5, align 8
  %7 = call i32 @atoi(i8* %6) #6
  store i32 %8, i32* @SIZE, align 4
  %10 = load i32, i32* @SIZE, align 4
  %11 = icmp sgt i32 %10, 1000000
  store i32 0, i32* %STEP, align 4
  %18 = load i32, i32* %STEP, align 4
  %19 = icmp slt i32 %18, 1
  %22 = load i32, i32* %STEP, align 4
  call void @initialize(i32* getelementptr inbounds ([1000000 x i32], [1000000 x i32]* @array, i32 0, i32 0), i32 %22)
  %1 = alloca i32*, align 8
  %2 = alloca i32, align 4
  %i = alloca i32, align 4
  store i32* %v, i32** %1, align 8
  store i32 %seed, i32* %2, align 4
  %3 = load i32, i32* %2, align 4
  call void @srandom(i32 %3) #5
  store i32 0, i32* %i, align 4
  %8 = load i32, i32* %i, align 4
  %9 = load i32, i32* @SIZE, align 4
  %10 = icmp ult i32 %8, %9
  %13 = call i64 @random() #6
  %16 = trunc i64 %14 to i32
  %17 = load i32, i32* %i, align 4
  %18 = zext i32 %17 to i64
  %19 = load i32*, i32** %1, align 8
  %20 = getelementptr inbounds i32, i32* %19, i64 %18
  store i32 %16, i32* %20, align 4
  %23 = load i32, i32* %i, align 4
  %24 = add i32 %23, 1
  store i32 %24, i32* %i, align 4
  %8 = load i32, i32* %i, align 4
  %9 = load i32, i32* @SIZE, align 4
  %10 = icmp ult i32 %8, %9
  %13 = call i64 @random() #6
  %16 = trunc i64 %14 to i32
  %17 = load i32, i32* %i, align 4
  %18 = zext i32 %17 to i64
  %19 = load i32*, i32** %1, align 8
  %20 = getelementptr inbounds i32, i32* %19, i64 %18
  store i32 %16, i32* %20, align 4
  %23 = load i32, i32* %i, align 4
  %24 = add i32 %23, 1
  store i32 %24, i32* %i, align 4
  %8 = load i32, i32* %i, align 4
  %9 = load i32, i32* @SIZE, align 4
  %10 = icmp ult i32 %8, %9
  %13 = call i64 @random() #6
  %16 = trunc i64 %14 to i32
  %17 = load i32, i32* %i, align 4
  %18 = zext i32 %17 to i64
  %19 = load i32*, i32** %1, align 8
  %20 = getelementptr inbounds i32, i32* %19, i64 %18
  store i32 %16, i32* %20, align 4
  %23 = load i32, i32* %i, align 4
  %24 = add i32 %23, 1
  store i32 %24, i32* %i, align 4
  %8 = load i32, i32* %i, align 4
  %9 = load i32, i32* @SIZE, align 4
  %10 = icmp ult i32 %8, %9
  %25 = load i32, i32* @SIZE, align 4
  %26 = sub nsw i32 %25, 1
  call void @qs(i32* getelementptr inbounds ([1000000 x i32], [1000000 x i32]* @array, i32 0, i32 0), i32 0, i32 %26)
store i32* %100, i32** %v, align 8
store  i32 %104, i32* %first, align 8
store  i32 %108, i32* %last, align 8
  %1 = alloca i32*, align 8
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %start = alloca [2 x i32], align 4
  %end = alloca [2 x i32], align 4
  %pivot = alloca i32, align 4
  %i = alloca i32, align 4
  %temp = alloca i32, align 4
  store i32* %v, i32** %1, align 8
  store i32 %first, i32* %2, align 4
  store i32 %last, i32* %3, align 4
  %4 = load i32, i32* %2, align 4
  %5 = load i32, i32* %3, align 4
  %6 = icmp slt i32 %4, %5
  %9 = load i32, i32* %2, align 4
  %10 = getelementptr inbounds [2 x i32], [2 x i32]* %start, i64 0, i64 1
  store i32 %9, i32* %10, align 4
  %11 = load i32, i32* %3, align 4
  %12 = getelementptr inbounds [2 x i32], [2 x i32]* %end, i64 0, i64 0
  store i32 %11, i32* %12, align 4
  %13 = load i32, i32* %2, align 4
  %14 = load i32, i32* %3, align 4
  %15 = add nsw i32 %13, %14
  %16 = sdiv i32 %15, 2
  %17 = sext i32 %16 to i64
  %18 = load i32*, i32** %1, align 8
  %19 = getelementptr inbounds i32, i32* %18, i64 %17
  %20 = load i32, i32* %19, align 4
  store i32 %20, i32* %pivot, align 4
  %23 = getelementptr inbounds [2 x i32], [2 x i32]* %start, i64 0, i64 1
  %24 = load i32, i32* %23, align 4
  %25 = getelementptr inbounds [2 x i32], [2 x i32]* %end, i64 0, i64 0
  %26 = load i32, i32* %25, align 4
  %27 = icmp sle i32 %24, %26
  %32 = getelementptr inbounds [2 x i32], [2 x i32]* %start, i64 0, i64 1
  %33 = load i32, i32* %32, align 4
  %34 = sext i32 %33 to i64
  %35 = load i32*, i32** %1, align 8
  %36 = getelementptr inbounds i32, i32* %35, i64 %34
  %37 = load i32, i32* %36, align 4
  %38 = load i32, i32* %pivot, align 4
  %39 = icmp slt i32 %37, %38
  %49 = load i32, i32* %pivot, align 4
  %50 = getelementptr inbounds [2 x i32], [2 x i32]* %end, i64 0, i64 0
  %51 = load i32, i32* %50, align 4
  %52 = sext i32 %51 to i64
  %53 = load i32*, i32** %1, align 8
  %54 = getelementptr inbounds i32, i32* %53, i64 %52
  %55 = load i32, i32* %54, align 4
  %56 = icmp slt i32 %49, %55
  %59 = getelementptr inbounds [2 x i32], [2 x i32]* %end, i64 0, i64 0
  %60 = load i32, i32* %59, align 4
  %61 = add nsw i32 %60, -1
  store i32 %61, i32* %59, align 4
  %49 = load i32, i32* %pivot, align 4
  %50 = getelementptr inbounds [2 x i32], [2 x i32]* %end, i64 0, i64 0
  %51 = load i32, i32* %50, align 4
  %52 = sext i32 %51 to i64
  %53 = load i32*, i32** %1, align 8
  %54 = getelementptr inbounds i32, i32* %53, i64 %52
  %55 = load i32, i32* %54, align 4
  %56 = icmp slt i32 %49, %55
  %64 = getelementptr inbounds [2 x i32], [2 x i32]* %start, i64 0, i64 1
  %65 = load i32, i32* %64, align 4
  %66 = getelementptr inbounds [2 x i32], [2 x i32]* %end, i64 0, i64 0
  %67 = load i32, i32* %66, align 4
  %68 = icmp sle i32 %65, %67
  %71 = getelementptr inbounds [2 x i32], [2 x i32]* %start, i64 0, i64 1
  %72 = load i32, i32* %71, align 4
  %73 = sext i32 %72 to i64
  %74 = load i32*, i32** %1, align 8
  %75 = getelementptr inbounds i32, i32* %74, i64 %73
  %76 = load i32, i32* %75, align 4
  store i32 %76, i32* %temp, align 4
  %77 = getelementptr inbounds [2 x i32], [2 x i32]* %end, i64 0, i64 0
  %78 = load i32, i32* %77, align 4
  %79 = sext i32 %78 to i64
  %80 = load i32*, i32** %1, align 8
  %81 = getelementptr inbounds i32, i32* %80, i64 %79
  %82 = load i32, i32* %81, align 4
  %83 = getelementptr inbounds [2 x i32], [2 x i32]* %start, i64 0, i64 1
  %84 = load i32, i32* %83, align 4
  %85 = sext i32 %84 to i64
  %86 = load i32*, i32** %1, align 8
  %87 = getelementptr inbounds i32, i32* %86, i64 %85
  store i32 %82, i32* %87, align 4
  %88 = load i32, i32* %temp, align 4
  %89 = getelementptr inbounds [2 x i32], [2 x i32]* %end, i64 0, i64 0
  %90 = load i32, i32* %89, align 4
  %91 = sext i32 %90 to i64
  %92 = load i32*, i32** %1, align 8
  %93 = getelementptr inbounds i32, i32* %92, i64 %91
  store i32 %88, i32* %93, align 4
  %94 = getelementptr inbounds [2 x i32], [2 x i32]* %start, i64 0, i64 1
  %95 = load i32, i32* %94, align 4
  %96 = add nsw i32 %95, 1
  store i32 %96, i32* %94, align 4
  %97 = getelementptr inbounds [2 x i32], [2 x i32]* %end, i64 0, i64 0
  %98 = load i32, i32* %97, align 4
  %99 = add nsw i32 %98, -1
  store i32 %99, i32* %97, align 4
  %23 = getelementptr inbounds [2 x i32], [2 x i32]* %start, i64 0, i64 1
  %24 = load i32, i32* %23, align 4
  %25 = getelementptr inbounds [2 x i32], [2 x i32]* %end, i64 0, i64 0
  %26 = load i32, i32* %25, align 4
  %27 = icmp sle i32 %24, %26
  %104 = load i32, i32* %2, align 4
  %105 = getelementptr inbounds [2 x i32], [2 x i32]* %start, i64 0, i64 0
  store i32 %104, i32* %105, align 4
  %106 = load i32, i32* %3, align 4
  %107 = getelementptr inbounds [2 x i32], [2 x i32]* %end, i64 0, i64 1
  store i32 %106, i32* %107, align 4
  store i32 0, i32* %i, align 4
  %110 = load i32, i32* %i, align 4
  %111 = icmp sle i32 %110, 1
  %114 = load i32*, i32** %1, align 8
  %115 = load i32, i32* %i, align 4
  %116 = sext i32 %115 to i64
  %117 = getelementptr inbounds [2 x i32], [2 x i32]* %start, i64 0, i64 %116
  %118 = load i32, i32* %117, align 4
  %119 = load i32, i32* %i, align 4
  %120 = sext i32 %119 to i64
  %121 = getelementptr inbounds [2 x i32], [2 x i32]* %end, i64 0, i64 %120
  %122 = load i32, i32* %121, align 4
  call void @qs(i32* %114, i32 %118, i32 %122)
store i32* %100, i32** %v, align 8
store  i32 %104, i32* %first, align 8
store  i32 %108, i32* %last, align 8
  %1 = alloca i32*, align 8
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %start = alloca [2 x i32], align 4
  %end = alloca [2 x i32], align 4
  %pivot = alloca i32, align 4
  %i = alloca i32, align 4
  %temp = alloca i32, align 4
  store i32* %v, i32** %1, align 8
  store i32 %first, i32* %2, align 4
  store i32 %last, i32* %3, align 4
  %4 = load i32, i32* %2, align 4
  %5 = load i32, i32* %3, align 4
  %6 = icmp slt i32 %4, %5
  store i32 0, i32* %i, align 4
  %131 = load i32, i32* %i, align 4
  store i32 %131, i32* %temp, align 4
  %132 = load i32, i32* %temp, align 4
  %133 = load i32, i32* %i, align 4
  %134 = add nsw i32 %132, %133
  store i32 %134, i32* %pivot, align 4
  %135 = getelementptr inbounds [2 x i32], [2 x i32]* %start, i64 0, i64 0
  store i32 1, i32* %135, align 4
  %136 = getelementptr inbounds [2 x i32], [2 x i32]* %start, i64 0, i64 1
  store i32 2, i32* %136, align 4
  %137 = getelementptr inbounds [2 x i32], [2 x i32]* %end, i64 0, i64 0
  store i32 3, i32* %137, align 4
  %138 = getelementptr inbounds [2 x i32], [2 x i32]* %end, i64 0, i64 1
  store i32 4, i32* %138, align 4
  %125 = load i32, i32* %i, align 4
  %126 = add nsw i32 %125, 1
  store i32 %126, i32* %i, align 4
  %110 = load i32, i32* %i, align 4
  %111 = icmp sle i32 %110, 1
  %114 = load i32*, i32** %1, align 8
  %115 = load i32, i32* %i, align 4
  %116 = sext i32 %115 to i64
  %117 = getelementptr inbounds [2 x i32], [2 x i32]* %start, i64 0, i64 %116
  %118 = load i32, i32* %117, align 4
  %119 = load i32, i32* %i, align 4
  %120 = sext i32 %119 to i64
  %121 = getelementptr inbounds [2 x i32], [2 x i32]* %end, i64 0, i64 %120
  %122 = load i32, i32* %121, align 4
  call void @qs(i32* %114, i32 %118, i32 %122)
store i32* %100, i32** %v, align 8
store  i32 %104, i32* %first, align 8
store  i32 %108, i32* %last, align 8
  %1 = alloca i32*, align 8
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %start = alloca [2 x i32], align 4
  %end = alloca [2 x i32], align 4
  %pivot = alloca i32, align 4
  %i = alloca i32, align 4
  %temp = alloca i32, align 4
  store i32* %v, i32** %1, align 8
  store i32 %first, i32* %2, align 4
  store i32 %last, i32* %3, align 4
  %4 = load i32, i32* %2, align 4
  %5 = load i32, i32* %3, align 4
  %6 = icmp slt i32 %4, %5
  %9 = load i32, i32* %2, align 4
  %10 = getelementptr inbounds [2 x i32], [2 x i32]* %start, i64 0, i64 1
  store i32 %9, i32* %10, align 4
  %11 = load i32, i32* %3, align 4
  %12 = getelementptr inbounds [2 x i32], [2 x i32]* %end, i64 0, i64 0
  store i32 %11, i32* %12, align 4
  %13 = load i32, i32* %2, align 4
  %14 = load i32, i32* %3, align 4
  %15 = add nsw i32 %13, %14
  %16 = sdiv i32 %15, 2
  %17 = sext i32 %16 to i64
  %18 = load i32*, i32** %1, align 8
  %19 = getelementptr inbounds i32, i32* %18, i64 %17
  %20 = load i32, i32* %19, align 4
  store i32 %20, i32* %pivot, align 4
  %23 = getelementptr inbounds [2 x i32], [2 x i32]* %start, i64 0, i64 1
  %24 = load i32, i32* %23, align 4
  %25 = getelementptr inbounds [2 x i32], [2 x i32]* %end, i64 0, i64 0
  %26 = load i32, i32* %25, align 4
  %27 = icmp sle i32 %24, %26
  %32 = getelementptr inbounds [2 x i32], [2 x i32]* %start, i64 0, i64 1
  %33 = load i32, i32* %32, align 4
  %34 = sext i32 %33 to i64
  %35 = load i32*, i32** %1, align 8
  %36 = getelementptr inbounds i32, i32* %35, i64 %34
  %37 = load i32, i32* %36, align 4
  %38 = load i32, i32* %pivot, align 4
  %39 = icmp slt i32 %37, %38
  %49 = load i32, i32* %pivot, align 4
  %50 = getelementptr inbounds [2 x i32], [2 x i32]* %end, i64 0, i64 0
  %51 = load i32, i32* %50, align 4
  %52 = sext i32 %51 to i64
  %53 = load i32*, i32** %1, align 8
  %54 = getelementptr inbounds i32, i32* %53, i64 %52
  %55 = load i32, i32* %54, align 4
  %56 = icmp slt i32 %49, %55
  %64 = getelementptr inbounds [2 x i32], [2 x i32]* %start, i64 0, i64 1
  %65 = load i32, i32* %64, align 4
  %66 = getelementptr inbounds [2 x i32], [2 x i32]* %end, i64 0, i64 0
  %67 = load i32, i32* %66, align 4
  %68 = icmp sle i32 %65, %67
  %71 = getelementptr inbounds [2 x i32], [2 x i32]* %start, i64 0, i64 1
  %72 = load i32, i32* %71, align 4
  %73 = sext i32 %72 to i64
  %74 = load i32*, i32** %1, align 8
  %75 = getelementptr inbounds i32, i32* %74, i64 %73
  %76 = load i32, i32* %75, align 4
  store i32 %76, i32* %temp, align 4
  %77 = getelementptr inbounds [2 x i32], [2 x i32]* %end, i64 0, i64 0
  %78 = load i32, i32* %77, align 4
  %79 = sext i32 %78 to i64
  %80 = load i32*, i32** %1, align 8
  %81 = getelementptr inbounds i32, i32* %80, i64 %79
  %82 = load i32, i32* %81, align 4
  %83 = getelementptr inbounds [2 x i32], [2 x i32]* %start, i64 0, i64 1
  %84 = load i32, i32* %83, align 4
  %85 = sext i32 %84 to i64
  %86 = load i32*, i32** %1, align 8
  %87 = getelementptr inbounds i32, i32* %86, i64 %85
  store i32 %82, i32* %87, align 4
  %88 = load i32, i32* %temp, align 4
  %89 = getelementptr inbounds [2 x i32], [2 x i32]* %end, i64 0, i64 0
  %90 = load i32, i32* %89, align 4
  %91 = sext i32 %90 to i64
  %92 = load i32*, i32** %1, align 8
  %93 = getelementptr inbounds i32, i32* %92, i64 %91
  store i32 %88, i32* %93, align 4
  %94 = getelementptr inbounds [2 x i32], [2 x i32]* %start, i64 0, i64 1
  %95 = load i32, i32* %94, align 4
  %96 = add nsw i32 %95, 1
  store i32 %96, i32* %94, align 4
  %97 = getelementptr inbounds [2 x i32], [2 x i32]* %end, i64 0, i64 0
  %98 = load i32, i32* %97, align 4
  %99 = add nsw i32 %98, -1
  store i32 %99, i32* %97, align 4
  %23 = getelementptr inbounds [2 x i32], [2 x i32]* %start, i64 0, i64 1
  %24 = load i32, i32* %23, align 4
  %25 = getelementptr inbounds [2 x i32], [2 x i32]* %end, i64 0, i64 0
  %26 = load i32, i32* %25, align 4
  %27 = icmp sle i32 %24, %26
  %104 = load i32, i32* %2, align 4
  %105 = getelementptr inbounds [2 x i32], [2 x i32]* %start, i64 0, i64 0
  store i32 %104, i32* %105, align 4
  %106 = load i32, i32* %3, align 4
  %107 = getelementptr inbounds [2 x i32], [2 x i32]* %end, i64 0, i64 1
  store i32 %106, i32* %107, align 4
  store i32 0, i32* %i, align 4
  %110 = load i32, i32* %i, align 4
  %111 = icmp sle i32 %110, 1
  %114 = load i32*, i32** %1, align 8
  %115 = load i32, i32* %i, align 4
  %116 = sext i32 %115 to i64
  %117 = getelementptr inbounds [2 x i32], [2 x i32]* %start, i64 0, i64 %116
  %118 = load i32, i32* %117, align 4
  %119 = load i32, i32* %i, align 4
  %120 = sext i32 %119 to i64
  %121 = getelementptr inbounds [2 x i32], [2 x i32]* %end, i64 0, i64 %120
  %122 = load i32, i32* %121, align 4
  call void @qs(i32* %114, i32 %118, i32 %122)
store i32* %100, i32** %v, align 8
store  i32 %104, i32* %first, align 8
store  i32 %108, i32* %last, align 8
  %1 = alloca i32*, align 8
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %start = alloca [2 x i32], align 4
  %end = alloca [2 x i32], align 4
  %pivot = alloca i32, align 4
  %i = alloca i32, align 4
  %temp = alloca i32, align 4
  store i32* %v, i32** %1, align 8
  store i32 %first, i32* %2, align 4
  store i32 %last, i32* %3, align 4
  %4 = load i32, i32* %2, align 4
  %5 = load i32, i32* %3, align 4
  %6 = icmp slt i32 %4, %5
  store i32 0, i32* %i, align 4
  %131 = load i32, i32* %i, align 4
  store i32 %131, i32* %temp, align 4
  %132 = load i32, i32* %temp, align 4
  %133 = load i32, i32* %i, align 4
  %134 = add nsw i32 %132, %133
  store i32 %134, i32* %pivot, align 4
  %135 = getelementptr inbounds [2 x i32], [2 x i32]* %start, i64 0, i64 0
  store i32 1, i32* %135, align 4
  %136 = getelementptr inbounds [2 x i32], [2 x i32]* %start, i64 0, i64 1
  store i32 2, i32* %136, align 4
  %137 = getelementptr inbounds [2 x i32], [2 x i32]* %end, i64 0, i64 0
  store i32 3, i32* %137, align 4
  %138 = getelementptr inbounds [2 x i32], [2 x i32]* %end, i64 0, i64 1
  store i32 4, i32* %138, align 4
  %125 = load i32, i32* %i, align 4
  %126 = add nsw i32 %125, 1
  store i32 %126, i32* %i, align 4
  %110 = load i32, i32* %i, align 4
  %111 = icmp sle i32 %110, 1
  %114 = load i32*, i32** %1, align 8
  %115 = load i32, i32* %i, align 4
  %116 = sext i32 %115 to i64
  %117 = getelementptr inbounds [2 x i32], [2 x i32]* %start, i64 0, i64 %116
  %118 = load i32, i32* %117, align 4
  %119 = load i32, i32* %i, align 4
  %120 = sext i32 %119 to i64
  %121 = getelementptr inbounds [2 x i32], [2 x i32]* %end, i64 0, i64 %120
  %122 = load i32, i32* %121, align 4
  call void @qs(i32* %114, i32 %118, i32 %122)
store i32* %100, i32** %v, align 8
store  i32 %104, i32* %first, align 8
store  i32 %108, i32* %last, align 8
  %1 = alloca i32*, align 8
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %start = alloca [2 x i32], align 4
  %end = alloca [2 x i32], align 4
  %pivot = alloca i32, align 4
  %i = alloca i32, align 4
  %temp = alloca i32, align 4
  store i32* %v, i32** %1, align 8
  store i32 %first, i32* %2, align 4
  store i32 %last, i32* %3, align 4
  %4 = load i32, i32* %2, align 4
  %5 = load i32, i32* %3, align 4
  %6 = icmp slt i32 %4, %5
  store i32 0, i32* %i, align 4
  %131 = load i32, i32* %i, align 4
  store i32 %131, i32* %temp, align 4
  %132 = load i32, i32* %temp, align 4
  %133 = load i32, i32* %i, align 4
  %134 = add nsw i32 %132, %133
  store i32 %134, i32* %pivot, align 4
  %135 = getelementptr inbounds [2 x i32], [2 x i32]* %start, i64 0, i64 0
  store i32 1, i32* %135, align 4
  %136 = getelementptr inbounds [2 x i32], [2 x i32]* %start, i64 0, i64 1
  store i32 2, i32* %136, align 4
  %137 = getelementptr inbounds [2 x i32], [2 x i32]* %end, i64 0, i64 0
  store i32 3, i32* %137, align 4
  %138 = getelementptr inbounds [2 x i32], [2 x i32]* %end, i64 0, i64 1
  store i32 4, i32* %138, align 4
  %125 = load i32, i32* %i, align 4
  %126 = add nsw i32 %125, 1
  store i32 %126, i32* %i, align 4
  %110 = load i32, i32* %i, align 4
  %111 = icmp sle i32 %110, 1
  %125 = load i32, i32* %i, align 4
  %126 = add nsw i32 %125, 1
  store i32 %126, i32* %i, align 4
  %110 = load i32, i32* %i, align 4
  %111 = icmp sle i32 %110, 1
  call void @testit(i32* getelementptr inbounds ([1000000 x i32], [1000000 x i32]* @array, i32 0, i32 0))
  %1 = alloca i32*, align 8
  %k = alloca i32, align 4
  %not_sorted = alloca i32, align 4
  store i32* %v, i32** %1, align 8
  store i32 0, i32* %not_sorted, align 4
  store i32 0, i32* %k, align 4
  %4 = load i32, i32* %k, align 4
  %5 = load i32, i32* @SIZE, align 4
  %6 = sub nsw i32 %5, 1
  %7 = icmp slt i32 %4, %6
  %10 = load i32, i32* %k, align 4
  %11 = sext i32 %10 to i64
  %12 = load i32*, i32** %1, align 8
  %13 = getelementptr inbounds i32, i32* %12, i64 %11
  %14 = load i32, i32* %13, align 4
  %15 = load i32, i32* %k, align 4
  %16 = add nsw i32 %15, 1
  %17 = sext i32 %16 to i64
  %18 = load i32*, i32** %1, align 8
  %19 = getelementptr inbounds i32, i32* %18, i64 %17
  %20 = load i32, i32* %19, align 4
  %21 = icmp sgt i32 %14, %20
  %28 = load i32, i32* %k, align 4
  %29 = add nsw i32 %28, 1
  store i32 %29, i32* %k, align 4
  %4 = load i32, i32* %k, align 4
  %5 = load i32, i32* @SIZE, align 4
  %6 = sub nsw i32 %5, 1
  %7 = icmp slt i32 %4, %6
  %10 = load i32, i32* %k, align 4
  %11 = sext i32 %10 to i64
  %12 = load i32*, i32** %1, align 8
  %13 = getelementptr inbounds i32, i32* %12, i64 %11
  %14 = load i32, i32* %13, align 4
  %15 = load i32, i32* %k, align 4
  %16 = add nsw i32 %15, 1
  %17 = sext i32 %16 to i64
  %18 = load i32*, i32** %1, align 8
  %19 = getelementptr inbounds i32, i32* %18, i64 %17
  %20 = load i32, i32* %19, align 4
  %21 = icmp sgt i32 %14, %20
  %28 = load i32, i32* %k, align 4
  %29 = add nsw i32 %28, 1
  store i32 %29, i32* %k, align 4
  %4 = load i32, i32* %k, align 4
  %5 = load i32, i32* @SIZE, align 4
  %6 = sub nsw i32 %5, 1
  %7 = icmp slt i32 %4, %6
  %32 = load i32, i32* %not_sorted, align 4
  %33 = icmp ne i32 %32, 0
  %38 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([15 x i8], [15 x i8]* @.str, i32 0, i32 0))
  %31 = load i32, i32* %STEP, align 4
  %32 = add nsw i32 %31, 1
  store i32 %32, i32* %STEP, align 4
  %18 = load i32, i32* %STEP, align 4
  %19 = icmp slt i32 %18, 1
  %35 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([15 x i8], [15 x i8]* @.str.1, i32 0, i32 0))
  %38 = load i32, i32* @SIZE, align 4
  %39 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([9 x i8], [9 x i8]* @.str.2, i32 0, i32 0), i32 %38, i32 1)
  %42 = load i32, i32* %1, align 4
