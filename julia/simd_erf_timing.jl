using CSV, DataFrames, Plots, LaTeXStrings, StatsPlots

function load_data()
    data_path = "./"*dirname(relpath(@__FILE__))*"/"
    data = CSV.File(data_path*"../csv/simd_erf.csv") |> DataFrame
    name = repeat(["Spline", "Spline Mirror", "Abramowitz", "Taylor", "SVML/STD"], outer=2)
    tmp = sum(Matrix(data); dims=1)
    res = reshape(tmp[2:end], (5,2))

    plt_erf = groupedbar(name, res, groups=repeat(["SIMD", "Sequential"], inner=5), ylabel="avg. cycles",
                        legend=:topleft, dpi=300)
    
    data = CSV.File(data_path*"../csv/simd_exp.csv") |> DataFrame
    name = repeat(["Spline", "Fast", "SVML/STD", "VCL"], outer=2)
    tmp = sum(Matrix(data); dims=1)
    res = tmp[2:end]
    append!(res, NaN)
    
    plt_exp = groupedbar(name, reshape(res, (4,2)), groups=repeat(["SIMD", "Sequential"], inner=4), ylabel="avg. cycles",
                        legend=:topleft, dpi=300)

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

