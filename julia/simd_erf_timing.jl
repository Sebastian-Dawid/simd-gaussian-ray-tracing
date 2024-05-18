using CSV, DataFrames, Plots, LaTeXStrings

function load_data()
    data_path = "./"*dirname(relpath(@__FILE__))*"/"
    data = CSV.File(data_path*"../csv/simd_erf.csv") |> DataFrame
    count = Vector(data[:, 1])
    t = Vector(data[:, 2])
    plt_erf = plot(count, t, dpi=300, label="simd", xlabel=L"$n$", ylabel=L"$ns$", title=L"Time to calculate $n$ values of $erf$.")
    t = Vector(data[:, 3])
    plot!(count, t, label="simd mirror")
    t = Vector(data[:, 4])
    plot!(count, t, label="spline")
    t = Vector(data[:, 5])
    plot!(count, t, label="spline mirror")
    t = Vector(data[:, 6])
    #plot!(count, t, label="std::erf")
    
    data = CSV.File(data_path*"../csv/simd_exp.csv") |> DataFrame
    count = Vector(data[:, 1])
    t = Vector(data[:, 2])
    plt_exp = plot(count, t, dpi=300, label="simd", xlabel=L"$n$", ylabel=L"$ns$", title=L"Time to calculate $n$ values of $\exp$.")
    t = Vector(data[:, 3])
    plot!(count, t, label="spline")
    t = Vector(data[:, 4])
    # plot!(count, t, label="std::exp")
    return plt_erf, plt_exp
end

if abspath(PROGRAM_FILE) == @__FILE__
    img_dir = "./"*dirname(relpath(@__FILE__))*"/../images"
    if !isdir(img_dir)
        mkdir(img_dir)
    end
    plt_erf, plt_exp = load_data()
    savefig(plt_erf, img_dir*"/simd_erf_timing.png")
    savefig(plt_exp, img_dir*"/simd_exp_timing.png")
end

