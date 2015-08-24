library(Rmisc)
library(ggplot2)
library(plyr)
library(scales)

pqplot <- function(csvfile) {
    # install.packages(c("Rmisc", "ggplot2", "plyr"))

    df <- read.csv(csvfile)
    colnames(df) <- c("kernel", "p", "throughput")

    df$throughput <- df$throughput/1E6

    df2 <- ddply(df, .(kernel, p), summarize, mean = mean(throughput),
                 lower = CI(throughput)[[3]], upper = CI(throughput)[[1]])

    # Bar graph

    # dodge <- position_dodge(width=0.9)
    # p <- ggplot(df2, aes(x = factor(p), y = mean, fill = factor(kernel))) +
    #             geom_bar(stat = "identity", position = dodge) +
    #             geom_errorbar(aes(ymin = lower, ymax = upper), position = dodge, width = 0.3) +
    #             ylab("throughput [Mops/s]") +
    #             xlab("number of threads")

    p <- ggplot(df2, aes(x = p, y = mean, color = kernel, shape = kernel)) +
                geom_line() +
                geom_point(size = 4) +
    #            geom_errorbar(aes(ymin = lower, ymax = upper), width = 0.3) +
                ylab("throughput in Mops/s") +
                xlab("number of threads") +
    # Themes
                theme_bw() +
                theme(axis.text = element_text(size = 16),
                      axis.title = element_text(size = 18),
                      legend.text = element_text(size = 16),
                      legend.title = element_text(size = 18),
                      legend.position = c(1, 1),
                      legend.justification = c(1, 1),
                      legend.background = element_rect(fill = alpha("black", 0)))

    figfile <- "fig.png"
    png(filename = figfile, width = 800, height = 600)
    plot(p)
    invisible(dev.off())
}

args <- commandArgs(trailingOnly = TRUE)
if (length(args) == 0) {
    f <- file("stdin")
} else if (length(args) == 1) {
    f <- args[1]
} else {
    stop("USAGE: Rscript plot.R [filename]")
}

pqplot(f)
