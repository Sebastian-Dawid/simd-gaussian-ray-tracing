using CSV, DataFrames, Plots, LaTeXStrings

function load_data()
    data_path = "./"*dirname(relpath(@__FILE__))*"/"
    data = CSV.File(data_path*"../csv/simd_erf.csv") |> DataFrame
    count = Vector(data[:, 1])
    t = Vector(data[:, 2])
    plt_erf = plot(count, t, dpi=300, label="simd spline", xlabel=L"$n$", ylabel=L"cycles", title=L"Cycles to calculate $n$ values of $erf$.", legend=:topleft)
    t = Vector(data[:, 3])
    plot!(count, t, label="simd spline mirror")
    t = Vector(data[:, 4])
    plot!(count, t, label="simd abramowitz stegun")
    t = Vector(data[:, 5])
    plot!(count, t, label="spline")
    t = Vector(data[:, 6])
    plot!(count, t, label="spline mirror")
    t = Vector(data[:, 7])
    plot!(count, t, label="abramowitz stegun")
    # t = Vector(data[:, 8])
    # plot!(count, t, label="std::erf")
    
    data = CSV.File(data_path*"../csv/simd_exp.csv") |> DataFrame
    count = Vector(data[:, 1])
    t = Vector(data[:, 2])
    plt_exp = plot(count, t, dpi=300, label="simd spline", xlabel=L"$n$", ylabel=L"cycles", title=L"Cycles to calculate $n$ values of $\exp$.", legend=:topleft)
    t = Vector(data[:, 3])
    plot!(count, t, label="spline")
    t = Vector(data[:, 4])
    plot!(count, t, label="fast_exp")
    t = Vector(data[:, 5])
    plot!(count, t, label="simd fast_exp")
    t = Vector(data[:, 6])
    plot!(count, t, label="std::exp")
    return plt_erf, plt_exp
end

if abspath(PROGRAM_FILE) == @__FILE__
    ext = ".tex"
    img_dir = "./"*dirname(relpath(@__FILE__))*"/../thesis/plots"
    if "--as-image" in ARGS || "-i" in ARGS
        ext = ".png"
        img_dir = "./"*dirname(relpath(@__FILE__))*"/../images"
    else
        pgfplotsx()
    end
    if !isdir(img_dir)
        mkdir(img_dir)
    end
    plt_erf, plt_exp = load_data()
    savefig(plt_erf, img_dir*"/"*ENV["ARCH"]*"_simd_erf_timing"*ext)
    savefig(plt_exp, img_dir*"/"*ENV["ARCH"]*"_simd_exp_timing"*ext)
end

