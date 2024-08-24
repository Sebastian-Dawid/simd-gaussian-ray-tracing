using CSV, DataFrames, Plots, LaTeXStrings

function load_data()
    data_path = "./"*dirname(relpath(@__FILE__))*"/"
    data = CSV.File(data_path*"../csv/timing_erf.csv") |> DataFrame
    count = Vector(data[:, 1])
    t = Vector(data[:, 2])
    plt_erf = plot(count, t, dpi=300, label="spline", xlabel="count", ylabel=L"$\mu s$", title="Time per pixel w.r.t. the number of gaussians\nin the scene (erf)")
    t = Vector(data[:, 3])
    plot!(count, t, label="mirror")
    t = Vector(data[:, 4])
    plot!(count, t, label="taylor")
    t = Vector(data[:, 5])
    plot!(count, t, label="abramowitz")
    t = Vector(data[:, 6])
    plot!(count, t, label="std::erf")
    
    data = CSV.File(data_path*"../csv/timing_exp.csv") |> DataFrame
    count = Vector(data[:, 1])
    t = Vector(data[:, 2])
    plt_exp = plot(count, t, dpi=300, label="spline", xlabel="count", ylabel=L"$\mu s$", title="Time per pixel w.r.t. the number of gaussians\nin the scene (exp)")
    t = Vector(data[:, 3])
    plot!(count, t, label="fast exp")
    t = Vector(data[:, 4])
    plot!(count, t, label="std::exp")
    
    data = CSV.File(data_path*"../csv/timing_transmittance.csv") |> DataFrame
    count = Vector(data[:, 1])
    t = Vector(data[:, 2])
    plt_trans = plot(count, t, dpi=300, label="simd", xlabel="count", ylabel=L"$\mu s$", title="Time per pixel w.r.t. the number of gaussians\nin the scene (exp)")
    t = Vector(data[:, 3])
    plot!(count, t, label="seq")
    return plt_erf, plt_exp, plt_trans
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
    plt_erf, plt_exp, plt_trans = load_data()
    savefig(plt_erf, img_dir*"/"*ENV["ARCH"]*"_timing_erf"*ext)
    savefig(plt_exp, img_dir*"/"*ENV["ARCH"]*"_timing_exp"*ext)
    savefig(plt_trans, img_dir*"/"*ENV["ARCH"]*"_timing_transmittance"*ext)
end
