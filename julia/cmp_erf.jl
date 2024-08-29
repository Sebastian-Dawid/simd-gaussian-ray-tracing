using CSV, DataFrames, Plots, LaTeXStrings

function load_data()
    data_path = "./"*dirname(relpath(@__FILE__))*"/"
    data = CSV.File(data_path*"../csv/erf.csv") |> DataFrame
    x = Vector(data[:, 1])
    y_spline = Vector(data[:, 2])
    erf_approx = plot(x, y_spline, dpi=300, label="spline", xlabel=L"x", ylabel=L"$y$", title="Approximations", legend=:topleft)
    y_mirror = Vector(data[:, 3])
    plot!(x, y_mirror, label="spline mirror")
    y_taylor = Vector(data[:, 4])
    plot!(x, y_taylor, label="taylor")
    y_abramowitz = Vector(data[:, 5])
    plot!(x, y_abramowitz, label="abramowitz stegun")
    y_svml = Vector(data[:, 6])
    plot!(x, y_svml, label="svml")
    y = Vector(data[:, 7])
    plot!(x, y, label="std::erf")
    erf_err = plot(x, abs.(y - y_spline), dpi=300, label="spline", xlabel=L"x", ylabel="err", title="Error", legend=:topleft)
    plot!(x, abs.(y - y_mirror), label="mirror")
    plot!(x, abs.(y - y_taylor), label="taylor")
    plot!(x, abs.(y - y_abramowitz), label="abramowitz stegun")
    plot!(x, abs.(y - y_svml), label="svml")

    data = CSV.File(data_path*"../csv/exp.csv") |> DataFrame
    x = Vector(data[:, 1])
    y_spline = Vector(data[:, 3])
    exp_approx = plot(x, y_spline, dpi=300, label="spline", xlabel=L"x", ylabel=L"$y$", title="Approximations", legend=:topleft)
    y_fast = Vector(data[:, 4])
    plot!(x, y_fast, label="fast")
    y_vcl = Vector(data[:, 5])
    plot!(x, y_vcl, label="vcl")
    y_svml = Vector(data[:, 6])
    plot!(x, y_svml, label="svml")
    y = Vector(data[:, 2])
    plot!(x, y, label="std::exp")
    exp_err = plot(x, abs.(y - y_spline), dpi=300, label="spline", xlabel=L"x", ylabel="err", title="Error", legend=:topleft)
    plot!(x, abs.(y - y_fast), label="fast")
    plot!(x, abs.(y - y_vcl), label="vcl")
    plot!(x, abs.(y - y_svml), label="svml")

    return erf_approx, erf_err, exp_approx, exp_err
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
    erf_approx, erf_err, exp_approx, exp_err = load_data()
    if "-i" in ARGS || "--as-image" in ARGS
        savefig(plot(erf_approx, erf_err), img_dir*"/"*ENV["ARCH"]*"_cmp_erf"*ext)
        savefig(plot(exp_approx, exp_err), img_dir*"/"*ENV["ARCH"]*"_cmp_exp"*ext)
    else
        savefig(erf_approx, img_dir*"/"*ENV["ARCH"]*"_cmp_erf_approx"*ext)
        savefig(erf_err, img_dir*"/"*ENV["ARCH"]*"_cmp_erf_err"*ext)
        savefig(exp_approx, img_dir*"/"*ENV["ARCH"]*"_cmp_exp_approx"*ext)
        savefig(exp_err, img_dir*"/"*ENV["ARCH"]*"_cmp_exp_err"*ext)
    end
end
