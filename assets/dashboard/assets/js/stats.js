$(function () {
    const dataBrut = $('#data-json');
    const dataJsonWrapper = $('#data-json-wrapper');

    $.get('/sensors.json', function (data) {
        const dataSorted = data.sort((a, b) => a < b ? 1 : -1);
        showData(dataSorted);

        const dataGraph = [];
        dataSorted.forEach(d => {
            dataGraph.push([
                moment(d.createdAt).toDate(), d.voltageBattery / 1000, d.voltageSolar / 1000, d.currentCharge, d.currentBattery, d.currentSolar
            ]);
        });

        const g = new Dygraph(
            document.getElementById("g"),
            dataGraph,
            {
                visibility: [true, false, true, false, false],
                legend: 'follow',
                labelsSeparateLines: ' ',
                strokeWidth: 2,
                highlightSeriesOpts: {
                    strokeWidth: 2,
                    highlightCircleSize: 5
                },
                title: 'Données de consommation',
                showRangeSelector: true,
                interactionModel: {
                    'mousedown': function downV3(event, g, context) {
                        context.initializeMouseDown(event, g, context);
                        if (event.altKey || event.shiftKey) {
                            Dygraph.startZoom(event, g, context);
                        } else {
                            Dygraph.startPan(event, g, context);
                        }
                    },
                    'mousemove': function moveV3(event, g, context) {
                        if (context.isPanning) {
                            Dygraph.movePan(event, g, context);
                        } else if (context.isZooming) {
                            Dygraph.moveZoom(event, g, context);
                        }
                    },
                    'mouseup': function upV3(event, g, context) {
                        if (context.isPanning) {
                            Dygraph.endPan(event, g, context);
                        } else if (context.isZooming) {
                            Dygraph.endZoom(event, g, context);
                        }
                    },
                    'click': function clickV3(event, g, context) {
                        event.preventDefault();
                    },
                    'mousewheel': function scrollV3(event, g, context) {
                        const percentages = offsetToPercentage(g, event.offsetX, event.offsetY);
                        const xPct = percentages[0];
                        const yPct = percentages[1];

                        zoom(g, event.wheelDelta / 1000, xPct, yPct);
                        event.preventDefault();
                    }
                },
                rollPeriod: 10,
                labels: ['Date', 'Voltage batterie', 'Voltage solaire', 'Intensité charge', 'Intensité batterie', 'Intensité solaire'],
                colors: ['#D00', '#FD0', '#00F', '#933', '#F93'],
                drawGrid: true,
                axes: {
                    y: {
                        drawAxesAtZero: true,
                        independentTicks: true,
                    },
                    y2: {
                        independentTicks: true,
                    }
                },
                series: {
                    'Intensité charge': {
                        axis: 'y2',
                    },
                    'Intensité batterie': {
                        axis: 'y2',
                    },
                    'Intensité solaire': {
                        axis: 'y2',
                    },
                },
                ylabel: 'Voltage (V)',
                y2label: 'Intensité (mA)',
            }
        );

        // Take the offset of a mouse event on the dygraph canvas and
        // convert it to a pair of percentages from the bottom left.
        // (Not top left, bottom is where the lower value is.)
        function offsetToPercentage(g, offsetX, offsetY) {
            // This is calculating the pixel offset of the leftmost date.
            const xOffset = g.toDomCoords(g.xAxisRange()[0], null)[0];
            const yar0 = g.yAxisRange(0);

            // This is calculating the pixel of the higest value. (Top pixel)
            const yOffset = g.toDomCoords(null, yar0[1])[1];

            // x y w and h are relative to the corner of the drawing area,
            // so that the upper corner of the drawing area is (0, 0).
            const x = offsetX - xOffset;
            const y = offsetY - yOffset;

            // This is computing the rightmost pixel, effectively defining the
            // width.
            const w = g.toDomCoords(g.xAxisRange()[1], null)[0] - xOffset;

            // This is computing the lowest pixel, effectively defining the height.
            const h = g.toDomCoords(null, yar0[0])[1] - yOffset;

            // Percentage from the left.
            const xPct = w === 0 ? 0 : (x / w);
            // Percentage from the top.
            const yPct = h === 0 ? 0 : (y / h);

            // The (1-) part below changes it from "% distance down from the top"
            // to "% distance up from the bottom".
            return [xPct, (1 - yPct)];
        }

        // Adjusts [x, y] toward each other by zoomInPercentage%
        // Split it so the left/bottom axis gets xBias/yBias of that change and
        // tight/top gets (1-xBias)/(1-yBias) of that change.
        //
        // If a bias is missing it splits it down the middle.
        function zoom(g, zoomInPercentage, xBias, yBias) {
            xBias = xBias || 0.5;
            yBias = yBias || 0.5;

            function adjustAxis(axis, zoomInPercentage, bias) {
                var delta = axis[1] - axis[0];
                var increment = delta * zoomInPercentage;
                var foo = [increment * bias, increment * (1 - bias)];
                return [axis[0] + foo[0], axis[1] - foo[1]];
            }

            var yAxes = g.yAxisRanges();
            var newYAxes = [];
            for (var i = 0; i < yAxes.length; i++) {
                newYAxes[i] = adjustAxis(yAxes[i], zoomInPercentage, yBias);
            }

            g.updateOptions({
                dateWindow: adjustAxis(g.xAxisRange(), zoomInPercentage, xBias),
                valueRange: newYAxes[0]
            });
        }

        $('#voltBattery').change(function (d) {
            g.setVisibility(0, $(this).is(":checked"));
        });
        $('#voltSolar').change(function (d) {
            g.setVisibility(1, $(this).is(":checked"));
        });
        $('#currentCharge').change(function (d) {
            g.setVisibility(2, $(this).is(":checked"));
        });
        $('#currentBattery').change(function (d) {
            g.setVisibility(3, $(this).is(":checked"));
        });
        $('#currentSolar').change(function (d) {
            g.setVisibility(4, $(this).is(":checked"));
        });
    });

    function showData(data) {
        console.log(data);
        dataBrut.html(JSON.stringify(data, null, 4));
        dataJsonWrapper.attr('open', '');
    }
});
