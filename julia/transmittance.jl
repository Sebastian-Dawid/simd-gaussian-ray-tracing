using Plots, CSV, DataFrames, LaTeXStrings

function load_data()
    data_path = "./"*dirname(relpath(@__FILE__))*"/"
    data = CSV.File(data_path*"../csv/data.csv") |> DataFrame
    s = Vector(data[:, 1])
    T = Vector(data[:, 2])
    T_s = Vector(data[:, 3])
    err = Vector(data[:, 4])
    D = Vector(data[:, 5])
    plot(s, T, label="analytic", dpi=300, xlabel=L"$s$", title=L"Transmittance and density along $o + sn$")
    plot!(s, T_s, label="approx")
    plot!(s, err, label="err")
    plot!(s, D, label="density")
end

if abspath(PROGRAM_FILE) == @__FILE__
    img_dir = "./"*dirname(relpath(@__FILE__))*"/../images"
    if !isdir(img_dir)
        mkdir(img_dir)
    end
    savefig(load_data(), img_dir*"/transmittance.png")
end
