using CSV, DataFrames, Plots, LaTeXStrings

function load_data()
    data_path = "./"*dirname(relpath(@__FILE__))*"/"
    data = CSV.File(data_path*"../csv/simd_erf.csv") |> DataFrame
    count = Vector(data[:, 1])
    t_svml = Vector(data[:, 2])
    t_simd_spline = Vector(data[:, 3])
    t_simd_spline_mirror = Vector(data[:, 4])
    t_simd_abramowitz = Vector(data[:, 5])
    t_spline = Vector(data[:, 6])
    t_spline_mirror = Vector(data[:, 7])
    t_abramowitz = Vector(data[:, 8])
    t_std = Vector(data[:, 9])

    plt_erf = plot(dpi=300, xlabel=L"$n$", ylabel=L"cycles", legend=:topleft)
    # plot!(count, t_std, label="std::erf")
    # plot!(count, t_spline, label="spline")
    # plot!(count, t_spline_mirror, label="spline mirror")
    # plot!(count, t_abramowitz, label="abramowitz stegun")
    plot!(count, t_svml, label="SVML")
    plot!(count, t_simd_spline, label="simd spline")
    plot!(count, t_simd_spline_mirror, label="simd spline mirror")
    plot!(count, t_simd_abramowitz, label="simd abramowitz stegun")
    
    data = CSV.File(data_path*"../csv/simd_exp.csv") |> DataFrame
    count = Vector(data[:, 1])
    t_vcl = Vector(data[:, 2])
    t_svml = Vector(data[:, 3])
    t_simd_spline = Vector(data[:, 4])
    t_spline = Vector(data[:, 5])
    t_fast = Vector(data[:, 6])
    t_simd_fast = Vector(data[:, 7])
    t_std = Vector(data[:, 8])
    plt_exp = plot(dpi=300, xlabel=L"$n$", ylabel=L"cycles", legend=:topleft)
    # plot!(count, t_std, label="std::exp")
    # plot!(count, t_spline, label="spline")
    # plot!(count, t_fast, label="fast_exp")
    plot!(count, t_svml, label="SVML")
    plot!(count, t_simd_spline, label="simd spline")
    plot!(count, t_simd_fast, label="simd fast_exp")
    plot!(count, t_vcl, label="vcl")
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

