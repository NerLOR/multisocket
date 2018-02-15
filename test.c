

#define MAX_NEURONS_PER_LAYER 64
#define MAX_NEURONS_IN_INPUT_LAYER 1024
#define MAX_LAYERS 16

typedef struct Neuron {
    float value;
    float setpoint;
    float bias;
    float weights[MAX_NEURONS_PER_LAYER];
} Neuron;

typedef struct InputNeuron {
    float value;
    float weights[MAX_NEURONS_PER_LAYER];
} InputNeuron;


typedef struct {
    struct InputLayer {
        neurons InputNeuron[MAX_NEURONS_IN_INPUT_LAYER];
    } inputLayer;
    struct Layer {
        Neuron neurons[MAX_NEURONS_PER_LAYER];
    } layers[MAX_LAYERS];
} Network;


int main() {
    Network a;
    a.layers[0].neurons[0].weights[0] = 0;
}

